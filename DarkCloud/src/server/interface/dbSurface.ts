import * as http from 'http';
import * as url from 'url';

import * as msgQueue from "../dataMovement/messageQueue"
import * as snapshot from "../dataMovement/snapshot"
import * as userControl from "../userManagment/userControl"
import * as dataIO from "../dataMovement/dataIO"

const surfacePort = 49204;

function invalidParameters(response: http.ServerResponse) {
    response.statusCode = 400;
    response.end('Invalid Parameter');
}

function handleRequest(request: http.IncomingMessage, response: http.ServerResponse) {
    //Parse URL
    const requestURL = url.parse(request.url, true);
    //All request would require sender user ID
    const userID: string = requestURL.query['uid'];
    //Phone tree for path name
    switch (requestURL.pathname) {
        case '/key': {
            if (request.method == "GET") {
                try {
                    let buf = userControl.queryPublicKey(userID);
                    response.statusCode = 200;
                    response.end(buf);
                    return;
                } catch (err) {}
            }
            return;
        }
        case '/message': {
            if (request.method == 'GET') {
                const mid = requestURL.query['mid'];
                msgQueue.getQueue(userID, mid, (err, data)=>{
                    if (err) {
                        response.statusCode = 500;
                        response.end();
                    } else {
                        response.statusCode = 200;
                        response.write(data)
                        response.end();
                    }
                });
                return;
            }
        }
        case '/queue': {
        //Queue will process
            switch (request.method) {
                case 'POST': {
                    var buffers: Buffer[] = [];
                    var length = 0;
                    request.on("data", (chunck: Buffer)=>{
                        buffers.push(chunck);
                        length += chunck.length;
                    });
                    request.on("end", ()=>{
                        const buffer = Buffer.concat(buffers, length);
                        const target: string = requestURL.query['tuid'];
                        const signature: string = request.headers["body-signature"];
                        msgQueue.messasgeInsertion(target, userID, buffer, signature, (err)=>{
                            if (err) {
                                //There is an error in push
                                response.statusCode = 500;
                            } else {
                                response.statusCode = 204;
                            }
                            response.end();
                        });
                    });
                    //Post a message to the queue
                    return;
                }
                case 'GET': {
                    if ( Number(requestURL.query['lastVer']) != NaN &&  Number(requestURL.query['offset']) != NaN) {
                        const lastVersion: number = requestURL.query['lastVer'];
                        const offset: number = requestURL.query['offset'];
                        msgQueue.queueRetrieve(userID, lastVersion, offset, (data, newVersion, newOffset, err)=>{
                            if (err) {
                                //Did not found the delta file
                                response.statusCode = 500;
                                response.end();
                            } else {
                                //Found the delta file
                                if (data.length > 0) {
                                    //There is new messages
                                    response.statusCode = 200;
                                    response.setHeader("Version", newVersion.toString());
                                    response.setHeader("Offset", newOffset.toString());
                                    response.end(data);
                                } else {
                                    response.statusCode = 204;
                                    response.end();
                                }
                            }
                        });
                    } else {
                        response.statusCode = 500;
                        response.end();
                    }
                    return;
                }
            }

            break;
        }
        case '/snapshot': {
            switch (request.method) {
                case 'PUT': {
                    //Vertification is done in the snapshot sub-routine
                    //Upload the snapshot
                    const userID: string = requestURL.query['uid'];
                    snapshot.startReplacement(userID, (err)=>{
                        //Something wrong, stop
                        if (err) {
                            request.removeAllListeners();
                            response.statusCode = 500;
                            response.end();
                        }
                    });
                    request.on('end', ()=>{
                        snapshot.solidfySnapshot(userID, (err)=>{
                            if (err) {
                                response.statusCode = 500;
                            } else {
                                //Store version
                                var v = new msgQueue.msgVersion;
                                v.revision = requestURL.query['ver'];
                                v.offset = requestURL.query['offset'];
                                dataIO.completeRewrite(userID, "snapshot.ver", new Buffer(v.toString()), (err)=>{});
                                response.statusCode = 204;
                            }
                            response.end();
                        });
                    });
                    request.on('data', (chunck: NodeBuffer)=> {
                        snapshot.updateSnapshot(userID, chunck, (err)=>{
                            //Somethign is wrong?
                            if (err) {
                                request.removeAllListeners();
                                response.statusCode = 500;
                                response.end();
                            }
                        });
                    });
                    return;
                }
                case 'GET': {
                    //Get the snapshot
                    snapshot.fetchSnapshot(userID, (err, buffer) => {
                        if (err) {
                            response.statusCode = 500;
                            response.end();
                        } else {
                            dataIO.completeRead(userID, "snapshot.ver", (data, err)=>{
                                if (err) {
                                    response.statusCode = 500;
                                    response.end();
                                } else {
                                    const v: msgQueue.msgVersion =  JSON.parse(data.toString('utf8'));
                                    response.setHeader("Version", v.revision.toString());
                                    response.setHeader("Offset", v.offset.toString());
                                    response.statusCode = 200;
                                    response.write(buffer);
                                    response.end();
                                }
                            })
                        }
                    });
                    return;
                }
            }
            break;
        }
        case '/init': {
            //Initial the account
            //Information needed: a user id hashed, and a user public certification
            switch (request.method) {
                case 'POST':
                {
                    request.on('data', (publicKeybag: NodeBuffer) => {
                        process.stderr.write(publicKeybag.length.toString());
                        if (publicKeybag) {
                            const UID = userControl.generateUserID(null);
                            userControl.userSetup(UID, publicKeybag, (err)=>{
                                if (err) {
                                    response.statusCode = 500;
                                } else {
                                    response.statusCode = 200;
                                    response.write(UID);
                                }
                                response.end();
                            });
                        }
                    });
                    return;
                }
            }
            break;
        }
    }
    response.statusCode = 404;
    response.end();
}

let server = http.createServer(handleRequest);

server.listen(surfacePort, function() {
    console.log("Start listening at %s", surfacePort);
})
