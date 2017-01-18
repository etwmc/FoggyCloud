import * as dataIO from "./dataIO";
import { ErrorObj, rollbackHandler } from "../../ErrorObj";

declare type fetchCallback = (ErrorObj, NodeBuffer)=>void;
declare type errorCallback = (err: ErrorObj)=>void;

//Snapshot: the last version of database file to speed up the restore process

export function fetchSnapshot(userID: string, callback: fetchCallback ) {
    dataIO.completeRead( userID, 'snapshot.db', (data: NodeBuffer, length: number, err: ErrorObj) => {
        if (err) err.appendCause('fetch snapshot');
        callback(err, data);
    });
}

export function initSnapshot(userID: string, callback: errorCallback) {
    if (dataIO.checkExistence(userID, "snapshot.db")) {
        callback(new ErrorObj("dataIO", "already exist"));
    } else {
        dataIO.touchFile(userID, "snapshot.db", (err, rollback)=>{
            if (err) {
                if (rollback) rollback();
                err.appendCause("init snapshot");
            }
            callback(err);
        });
    }
}

export function startReplacement(userID: string, callback: errorCallback) {
    //Need to check identity
    dataIO.resetFile(userID, 'snapshot.new.db', (err, rollback)=>{
        if (err) err.appendCause('start replacement');
        callback(err);
    })
}

export function updateSnapshot(userID: string, snapshot: NodeBuffer, callback: errorCallback) {
    //Need to check identity
    dataIO.appendData(userID, "snapshot.new.db", snapshot, (err, rollback) => {
        if (err) err.appendCause('update snapshot');
        callback(err);
    });
}

export function solidfySnapshot(userID: string, callback: errorCallback) {
    //Need to check identity, and correctness
    dataIO.replaceFile(userID, "snapshot.old.db", "snapshot.db", (err, rollback)=>{
        if (err) {
            err.appendCause('soldify snapshot - preserve');
            callback(err);
        }
        else dataIO.replaceFile(userID, "snapshot.db", "snapshot.new.db", (err, rollback) => {
            if (err) err.appendCause("soldify snapshot - replace");
            callback(err);
        })
    });
}
