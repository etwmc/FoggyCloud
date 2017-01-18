//Data IO is a class to encasuplate the actual file interaction

import * as path from 'path';
import * as fs from 'fs';
import * as buffer from "buffer";
import { ErrorObj, rollbackHandler } from "../../ErrorObj";

const _lockFile = require('lockfile');
const Promise = require("bluebird");

declare type dataErrorHandler = (err: ErrorObj, rollback: rollbackHandler) => void;
declare type dataReturnHandler = (data: NodeBuffer, length: number, err: ErrorObj, rollback: rollbackHandler) => void;

//Classic Single Server, file path based access

const rootFolder = "./testbed/"

fs.mkdir(rootFolder, 460, (err)=>{
	if (err) process.stderr.write(err.toString()+"\n");
});

function recordAddressIdentity(userID: string, filename: string) {
	return path.join(rootFolder, userID, filename);
}

export function checkExistence(userID: string, filename: string):boolean {
	const addr = recordAddressIdentity(userID, filename);
	return fs.existsSync(addr);
}

export function uniqueCreateAndLock_Promise(userID: string, filename: string):Promise<any> {
	return new Promise((res, rej)=>{
		uniqueCreateAndLock(userID, filename, (err, rollback)=>{
			if (err) rej({err: err});
			else res({rollback: rollback});
		});
	})
}
export function uniqueCreateAndLock(userID: string, filename: string, callback: dataErrorHandler) {
	if (checkExistence(userID, filename)) callback(new ErrorObj("dataIO", "file exist"), null);
	touchFile(userID, filename, (err, rollback) => {
		if (err) {
			err.appendCause("dataIO");
			callback(err, rollback);
		} else {
			lockFile(userID, filename, (err, rollback) => {
				if (err) {
					err.appendCause("dataIO");
					callback(err, rollback);
				} else callback(null, rollback);
			})
		}
	});
}

export function lockFile_Promise(userID: string, filename: string): Promise<any> {
	return new Promise((res, rej)=>{
		lockFile(userID, filename, (err, rollback)=>{
			if (err) rej({err: err});
			else res({rollback: rollback});
		});
	});
}
export function lockFile(userID: string, filename: string, callback: dataErrorHandler) {
	const addr = recordAddressIdentity(userID, filename+".lock");
	_lockFile.lock(addr, (err)=>{
		if (err) callback(new ErrorObj("dataIO", err), null);
		else callback(null, null);
	});
}

export function unlockFile_Promise(userID: string, filename: string): Promise<any> {
	return new Promise((res, rej)=>{
		unlockFile(userID, filename, (err, rollback)=>{
			if (err) rej({err: err});
			else res({rollback: rollback});
		});
	});
}
export function unlockFile(userID: string, filename: string, callback: dataErrorHandler) {
	const addr = recordAddressIdentity(userID, filename+".lock");
	_lockFile.unlock(addr, (err)=>{
		if (err) callback(new ErrorObj("dataIO", err), null);
		else callback(null, null);
	});
}

export function touchFile_Promise(userID: string, filename: string): Promise<any> {
	return new Promise((res, rej)=>{
		touchFile(userID, filename, (err, rollback)=>{
			if (err) rej({err: err});
			else res({rollback: rollback});
		});
	});
}
export function touchFile(userID: string, filename: string, callback: dataErrorHandler, rollback: rollbackHandler = null) {
	process.stderr.write("Touching File\n");
	const addr = recordAddressIdentity(userID, filename);
	fs.appendFile(addr, "", (err) => {
		process.stderr.write("Touchi File\n");
		const roll = rollback?  ()=>{
			removeFile(userID, filename, (err)=>{
				process.stderr.write("DataIO.Touch File rollback fail: remove fie");
			});
			rollback();
		}: ()=>{
			removeFile(userID, filename, (err)=>{
				process.stderr.write("DataIO.Touch File rollback fail: remove fie");
			});
		};
		if (err) {
			if (rollback) rollback();
			callback(new ErrorObj('dataIO', err), roll);
		} else callback(null, roll);
	});
}

//Append data to the user.
//Callback: function(error)
//Callback will be fired only once, either it fails or success
export function appendData_Promise(userID: string, filename: string, data: NodeBuffer): Promise<any> {
	return new Promise((res, rej)=>{
		appendData(userID, filename, data, (err, rollback)=>{
			if (err) rej({err: err});
			else res({rollback: rollback});
		});
	});
}
export function appendData(userID: string, filename: string, data: NodeBuffer, callback: dataErrorHandler, rollback: rollbackHandler = null ) {
	const dataPath = recordAddressIdentity(userID, filename);
	fs.appendFile(dataPath, data, (err)=>{
		if (err) {
			const roll = rollback?  ()=>{
				removeFile(userID, filename, (err)=>{
					process.stderr.write("DataIO.Touch File rollback fail: remove fie");
				});
				rollback();
			}: ()=>{
				removeFile(userID, filename, (err)=>{
					process.stderr.write("DataIO.Touch File rollback fail: remove fie");
				});
			};
			if (rollback) rollback();
			callback(new ErrorObj('dataIO', err), rollback);
		} else callback(null, rollback);
	})
}

//There is no size limit on the function itself. However, it is run in sync, so large file will heavily block the function
export function smallSizeSyncRead(userID: string, filename: string): NodeBuffer {
	const dataPath = recordAddressIdentity(userID, filename);
	return fs.readFileSync(dataPath);
}

export function read_Promise(userID: string, filename: string, offset: number): Promise<any> {
	return new Promise((res, rej)=>{
		read(userID, filename, offset, (data, length, err, rollback)=>{
			if (err) rej({err: err});
			else res({rollback: rollback, data: data, length: length});
		});
	});
}
export function read(userID: string, filename: string, offset: number, callback: dataReturnHandler, rollback: rollbackHandler = null ) {
	const dataPath = recordAddressIdentity(userID, filename);
	fs.open(dataPath, 'r', (err, fd)=>{
		if (err)
			callback(null, 0, new ErrorObj("dataIO", err), null);
		else
			fs.fstat(fd, function(err, stats) {
				if (err) callback(null, 0, new ErrorObj("dataIO", err), null);
				else {
					const bufferSize=stats.size - offset;
					var buffer=new Buffer(bufferSize);


					fs.read(fd, buffer, 0, bufferSize, offset, (err, byteRead, buffer)=>{
						fs.close(fd, (err)=>{
							if (err) process.stderr.write(err.toString());
						});
						if (err) callback(null, 0, new ErrorObj("dataIO", err), null);
						else callback(buffer, byteRead, null, null);
					});
				}
			});
	});
	fs.readFile(dataPath, (err, data) => {
		if (err) {
			if (rollback) rollback();
			callback(null, 0, new ErrorObj('dataIO', err), rollback);
		}
		else callback(data, data.length, null, rollback);
	});
}

export function completeRead_Promise(userID: string, filename: string): Promise<any> {
	return new Promise((res, rej)=>{
		completeRead(userID, filename, (data, length, err, rollback)=>{
			if (err) rej({err: err});
			else res({rollback: rollback, data: data, length: length});
		});
	});
}
export function completeRead(userID: string, filename: string, callback: dataReturnHandler, rollback: rollbackHandler = null ) {
	const dataPath = recordAddressIdentity(userID, filename);
	fs.readFile(dataPath, (err, data) => {
		if (err) {
			if (rollback) rollback();
			callback(null, 0, new ErrorObj('dataIO', err), rollback);
		}
		else callback(data, data.length, null, rollback);
	});
}

export function removeFile_Promise(userID: string, filename: string): Promise<any> {
	return new Promise((res, rej)=>{
		removeFile(userID, filename, (err, rollback)=>{
			if (err) rej({err: err});
			else res({rollback: rollback});
		});
	});
}
export function removeFile(userID: string, filename: string, callback: dataErrorHandler, rollback: rollbackHandler = null ) {
	const dataPath = recordAddressIdentity(userID, filename);
	if (fs.existsSync(dataPath)) {
		fs.unlink(dataPath, (err)=>{
			if (err) {
				if (rollback) rollback();
				callback(new ErrorObj('dataIO', err), rollback);
			}
			else callback(null,rollback);
		});
	}
}

export function resetFile_Promise(userID: string, filename: string): Promise<any> {
	return new Promise((res, rej)=>{
		resetFile(userID, filename, (err, rollback)=>{
			if (err) rej({err: err});
			else res({rollback: rollback});
		});
	});
}
export function resetFile(userID: string, filename: string, callback: dataErrorHandler, rollback: rollbackHandler = null ) {
	const dataPath = recordAddressIdentity(userID, filename);
	if (fs.existsSync(dataPath)) {
		fs.unlinkSync(dataPath);
	}
	fs.appendFile(dataPath, "", (err) => {
		if (err) {
			if (rollback) rollback();
			callback(new ErrorObj('dataIO', err), rollback);
		} else callback(null, rollback);
	});
}

export function replaceFile_Promise(userID: string, filename: string, oldFile: string): Promise<any> {
	return new Promise((res, rej)=>{
		replaceFile(userID, filename, oldFile, (err, rollback)=>{
			if (err) rej({err: err});
			else res({rollback: rollback});
		});
	});
}
export function replaceFile(userID: string, filename: string, oldFile: string, callback: dataErrorHandler, rollback: rollbackHandler = null ) {
	const dataPath = recordAddressIdentity(userID, filename);
	const oldDataPath = recordAddressIdentity(userID, oldFile);
	fs.rename(oldDataPath, dataPath, (err)=> {
		if (err) {
			if (rollback) rollback();
			callback(new ErrorObj('dataIO', err), rollback);
		} else callback(null, rollback);
	});
}

export function createFolder_Promise(userID: string, folderPath: string): Promise<any> {
	return new Promise((res, rej)=>{
		createFolder(userID, folderPath, (err, rollback)=>{
			if (err) rej({err: err});
			else res({rollback: rollback});
		});
	});
}
export function createFolder(userID: string, folderPath: string, callback: dataErrorHandler, rollback: rollbackHandler = null ) {
	const dataPath = recordAddressIdentity(userID, folderPath);
	fs.mkdir(dataPath, 460, (err)=>{
		if (err) {
			if (rollback) rollback();
			callback(new ErrorObj('dataIO', err), rollback);
		}
		else callback(null, rollback);
	});

}

//Completely overwrite the file with data
//Callback: function(error)
//Callback will be fired at most once, only when it fails
export function completeRewrite_Promise(userID: string, filename: string, data: NodeBuffer, callback: dataErrorHandler, rollback: rollbackHandler = null ): Promise<any> {
	return new Promise((res, rej)=>{
		completeRewrite(userID, filename, data, (err, rollback)=>{
			if (err) rej({err: err});
			else res({rollback: rollback});
		});
	});
}
export function completeRewrite(userID: string, filename: string, data: NodeBuffer, callback: dataErrorHandler, rollback: rollbackHandler = null ) {
	const dataPath = recordAddressIdentity(userID, filename);
	return fs.writeFile(dataPath, data, (err)=>{
		if (err) {
			if (rollback) rollback();
			callback(new ErrorObj('dataIO', err), rollback);
		}
		callback(null, rollback);
	})
}
