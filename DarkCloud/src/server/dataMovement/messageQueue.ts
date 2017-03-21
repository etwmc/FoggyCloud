import * as dataIO from "./dataIO";
import { ErrorObj, rollbackHandler } from "../../ErrorObj"
const uuidV1 = require('uuid/v1');

const Promise = require("bluebird");

declare type msgOpErrorHandler = (errType: ErrorObj) => void;
declare type msgDataErrorHandler = (errType: ErrorObj, data: NodeBuffer) => void;

export function getPublicKey(userID: string) {
    //Get the key bag, and fetch
    const keybagData = dataIO.smallSizeSyncRead(userID, 'keybag');
    if (keybagData.length == 0) throw "No Public Key";
    const jsonDict = JSON.parse(keybagData.toString());
    return jsonDict['PubKey'];
}

export class msgVersion {
    revision: number = 0;
    offset: number = 0;
}

export class syncVersion {
    currentVersion: msgVersion;
    fileTrim: number = 0;
    persistedVersionOffset: Map<number, number> = new Map<number, number>();
    toString() {
        return JSON.stringify(this);
    }
}

export function getVersion(userID: string): syncVersion {
    const verStr = dataIO.smallSizeSyncRead(userID, 'version');
    const verDict: syncVersion = JSON.parse(verStr.toString());
    return verDict;
}

export function setVersion(userID: string, version: syncVersion, callback: msgOpErrorHandler) {
    dataIO.completeRewrite(userID, 'version', new Buffer(version.toString()), (err)=>{
        if (err) err.appendCause("msgQueue");
        callback(err);
    }, null)
}

//Two tasks:
//1. Append the UID and SID to the message queue, so it could to retrieved afterward
//2. push to the connected devices so they don't need periodically
//Callback: to perform async write on all backend. Push should have no callback: if it fail, do a periodic check on how many message got in the time
//Callback prototype: function(errorSource, error)
//Callback parameter: errorSource represent which the error comes from
//Callback will be fired at least once, regardless if it sucess
function pushMessage(userID: string, messageID: string, senderID: string, callback: msgOpErrorHandler) {
    //Write the data
    //Push messages if nothing happen
    dataIO.lockFile(userID, "msgQueue", (err)=>{
        if (err) {
            err.appendCause("msgQueue");
            callback(err);
        } else {
            const packet = {
                sid: senderID,
                mid: messageID,
                time: (new Date()).getTime(),
            }
            dataIO.appendData(userID, "msgQueue", new Buffer(JSON.stringify(packet)), (err, rollback)=>{
                dataIO.unlockFile(userID, "msgQueue", (err)=>{});
                if (err) err.appendCause("msgQueue");
                callback(err);
            }, null);
            //When it is done, release
        }
    });

}

//Message requirement:
//0-31: binary AES-256 key first wrap with sender public key, then wrap with receiver public key
//32-...: AES-256 with padding, then HMAC-SHA1 (SHA256 later)
//Handle: directly write to a random file
//Signature: SHA512 for the message packet in base64
//After checking signature, it will write async with a UUID name, and an async write the UUID in push

//The data would be encrypted using the receiver public key in RSA on the user end, then sign by the sender's private key
//At the user side, it will be opague to the server, only can be decrypt using receiver's private key, which is PRIVATE to the device end. So it ensure end to end encrpytion between the sender and receiver
//Callback prototype: function(errorSource, error)
//Callback will be fired at least once, regardless if it sucess
export function messasgeInsertion(targetUserID: string, senderIdentityID: string, msgID: string, message: NodeBuffer, signature: string, callback: msgOpErrorHandler) {
    //Data needed for receiver to verify and decrypt the message on the device from the server:
    //sender's public key, which can be retrieved by sender identity ID
    //the packet itself

    dataIO.uniqueCreateAndLock(targetUserID, "msg/"+msgID, (err)=>{
        if (err) {
            //Retry
            err.appendCause("msgQueue");
            callback(err);
            //messasgeInsertion(targetUserID, senderIdentityID, message, signature, callback);
        } else {
            //No problem, start
            //Write the object
            var counter = -2;
            const merge = function() {
                if (counter == 0) {
                    //Actually running
                    callback(null);
                }
            }
            dataIO.appendData(targetUserID, "msg/"+msgID, message, (err)=>{
                dataIO.unlockFile(targetUserID, "msg/"+msgID, (err)=>{});
                if (err) {
                    err.appendCause("msgQueue");
                    callback(err);
                } else {
                    counter++
                    merge();
                }
            })
            pushMessage(targetUserID, msgID, senderIdentityID, (err)=>{
                if (err) {
                    err.appendCause("msgQueue");
                    callback(err);
                }else {
                    counter++
                    merge();
                }
            });
        }
    })

}

function getRecordVersion(): number {
    return 1;
}

function calculateActualOffset(userID: string, revision: number, offset: number):number {
    const verDict = getVersion(userID);
    return revision - verDict.fileTrim;
}

function calculateVersion(userID: string, revision: number, offset: number):msgVersion {
    const verDict = getVersion(userID);
    const ver = new msgVersion();
    ver.revision = 0;
    ver.offset = offset - verDict.fileTrim;
    return ver;
}

//It will calculate the offset from the last update, and return all of that with callback
//Callback: function(data, errorType, error)
export function queueRetrieve(userID: string, revision: number, offset: number=0, callback: ((msg: NodeBuffer, newVersion: number, newOffset: number, error: ErrorObj)=>void)) {
    //WHen fetch stuff, lock
    dataIO.lockFile(userID, "msgQueue", (err)=>{
        const deltaOffset = calculateActualOffset(userID, revision, offset);
        if (deltaOffset >= 0) {
            dataIO.read(userID, "msgQueue", deltaOffset, (data, len, err, roll)=>{
                dataIO.unlockFile(userID, "msgQueue", (err)=>{});
                if (err) {
                    err.appendCause("msgQueue");
                } else {
                    const newVersion = calculateVersion(userID, revision, offset+len);
                    callback(data, newVersion.revision, newVersion.offset, null);
                }
            });
        } else {
            callback(null, null, null, new ErrorObj("msgQueue", "obslete version, require full reload"));
            dataIO.unlockFile(userID, "msgQueue", (err)=>{});
        }
    });
}

export function getQueue(userID: string, mid: string, callback: msgDataErrorHandler) {
    dataIO.completeRead(userID, "msg/"+mid, (data, len, err, rollback)=>{
        if (err) err.appendCause("msgQueue");
        callback(err, data);
    })
}
