import * as crypto from "crypto"
import * as dataIO from "../dataMovement/dataIO"
import * as snapshot from "../dataMovement/snapshot"
import * as msgQueue from "../dataMovement/messageQueue"
import { ErrorObj } from "../../ErrorObj"
const uuidV4 = require('uuid/v4');

const hashAlg = crypto.createHash('sha512');

declare type noOutputErrorHandler = (err: ErrorObj) => void;

export function generateUserID(loginInfo: [string]): string {
    return uuidV4();
}

export function userSetup(userID: string, publicKeybag: NodeBuffer, _callback: noOutputErrorHandler) {
    const callback: noOutputErrorHandler = (err)=>{
    process.stdout.write("Done");
        if (err) {
            dataIO.removeFile(userID, "", (err)=>{
                process.stderr.write("Rollback error on user setup: "+err.toString()+"\n");
            });
        }
        _callback(err);
    }
    dataIO.createFolder(userID, "", (err)=>{
        if (err) {
            err.appendCause("User Setup");
            callback(err);
        } else {
            dataIO.completeRewrite(userID, "public.keybag", publicKeybag, (err)=>{
                if (err) {
                    err.appendCause("User Setup");
                    callback(err);
                }
                else {
                    snapshot.initSnapshot(userID, (err)=>{
                        if (err) {
                            err.appendCause("User Setup");
                        } else {
                            dataIO.createFolder(userID, "msg", (err)=>{
                                if (err) err.appendCause("userCtrl");
                                callback(err);
                            })
                        }
                    });
                }
            });
            msgQueue.setVersion(userID, new msgQueue.syncVersion(), (err)=>{

            })
        }
    });
}

export function queryPublicKey(uid: string): NodeBuffer {
    return dataIO.smallSizeSyncRead(uid, "public.keybag");
}
