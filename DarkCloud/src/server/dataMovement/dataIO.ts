//Data IO is a class to encasuplate the actual file interaction

import * as path from 'path';
import * as fs from 'fs';
import * as buffer from "buffer";
import { ErrorObj } from "../../ErrorObj";

declare type dataErrorHandler = (err: ErrorObj) => void;
declare type dataReturnHandler = (data: NodeBuffer, length: number, err: ErrorObj) => void;

//Classic Single Server, file path based access

const rootFolder = "./testbed/"

function recordAddressIdentity(userID: string, filename: string) {
	return path.join(rootFolder, userID, filename);
}

//Append data to the user.
//Callback: function(error)
//Callback will be fired at most once, only when it fails
export function appendData(userID: string, filename: string, data: NodeBuffer, callback: dataErrorHandler ) {
	const dataPath = recordAddressIdentity(userID, filename);
	fs.appendFile(dataPath, data, (err)=>{
		if (err) callback(new ErrorObj('dataIO', err));
	})
}

//There is no size limit on the function itself. However, it is run in sync, so large file will heavily block the function
export function smallSizeSyncRead(userID: string, filename: string): NodeBuffer {
	const dataPath = recordAddressIdentity(userID, filename);
	return fs.readFileSync(dataPath);
}

export function completeRead(userID: string, filename: string, callback: dataReturnHandler) {
	const dataPath = recordAddressIdentity(userID, filename);
	fs.readFile(dataPath, (err, data) => {
		if (err) callback(null, 0, new ErrorObj('dataIO', err));
		else callback(data, data.length, null);
	});
}

//Completely overwrite the file with data
//Callback: function(error)
//Callback will be fired at most once, only when it fails
export function completeRewrite(userID: string, filename: string, data: NodeBuffer, callback: dataErrorHandler) {
	const dataPath = recordAddressIdentity(userID, filename);
	return fs.writeFile(dataPath, data, (err)=>{
		if (err) callback(new ErrorObj('dataIO', err));
	})
}
