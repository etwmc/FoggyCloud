import * as crypto from 'crypto'
import * as dataIO from "./dataIO";

declare type msgOpErrorHandler = (errType: string, err: string) => void;

export function getPublicKey(userID: string) {
    //Get the key bag, and fetch
    const keybagData = dataIO.smallSizeSyncRead(userID, 'keybag');
    if (keybagData.length == 0) throw "No Public Key";
    const jsonDict = JSON.parse(keybagData.toString());
    return jsonDict['PubKey'];
}

export class syncVersion {
    currentDBVersion: number;
    persistedVersionOffset: [number, number];
}

export function getVersion(userID: string): syncVersion {
    const verStr = dataIO.smallSizeSyncRead(userID, 'version');
    const verDict: syncVersion = JSON.parse(verStr.toString());
    return verDict;
}

//Two tasks:
//1. Append to the message queue, so it could to retrieved afterward
//2. push to the connected devices so they don't need periodically
//Callback: to perform async write on all backend. Push should have no callback: if it fail, do a periodic check on how many message got in the time
//Callback prototype: function(errorSource, error)
//Callback parameter: errorSource represent which the error comes from
//Callback will be fired at least once, regardless if it sucess
export function pushMessage(userID: string, message: NodeBuffer, callback) {
    //Write the data
    dataIO.appendData(userID, 'msgQueue', message, (err)=> {
        if (err) {
            err.appendCause('messageQueue');
            callback('writeback', err);
        }
    //Push messages if nothing happen
        else {
        //Push
            callback(null, null)
        }
    });
}

//The data would be encrypted using the receiver public key in RSA, then sign by the sender's private key
//At the user side, it will be opague to the server, only can be decrypt using receiver's private key, which is PRIVATE to the device end. So it ensure end to end encrpytion between the sender and receiver
//Callback prototype: function(errorSource, error)
//Callback will be fired at least once, regardless if it sucess
export function messasgeInsertion(targetUserID, senderIdentityID, data, callback) {
    //Data needed for receiver to verify and decrypt the message on the device from the server:
    //sender's public key, which can be retrieved by sender identity ID
    //the packet itself
    let messagePacket = {
    sender: senderIdentityID,
    data: data
    };
    let messagePacketStr = JSON.stringify(messagePacket);
    //After wrapping the packet, encrypt the packet using sender public key so there is no metadata to see
    const receiverPubKey = getPublicKey(targetUserID);
    if (receiverPubKey === null) {
        callback('security', 'No Public Key found');
        return;
    }
    const resultedPacket = crypto.publicEncrypt(receiverPubKey, Buffer.from(messagePacketStr) );
    pushMessage(targetUserID, resultedPacket, callback);
}

//It will calculate the offset from the last update, and return all of that with callback
//Callback: function(data, errorType, error)
export function queueRetrieve(userID: string, revision: number, offset: number=0, callback: ((data: NodeBuffer, newVersion: number, newOffset: number, errorType: string, error: string)=>void)) {
}
