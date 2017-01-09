import * as dataIO from "./dataIO";
import { ErrorObj } from "../../ErrorObj"

declare type fetchCallback = (ErrorObj, NodeBuffer)=>void;
declare type errorCallback = (err: ErrorObj)=>void;

export function fetchSnapshot(userID: string, callback: fetchCallback ) {
    dataIO.completeRead( userID, 'snapshot', (data: NodeBuffer, length: number, err: ErrorObj) => {
        if (err) err.appendCause('snapshot');
        callback(err, data);
    });
}

export function updateSnapshot(userID: string, snapshot: NodeBuffer, callback: errorCallback) {
    dataIO.
}
