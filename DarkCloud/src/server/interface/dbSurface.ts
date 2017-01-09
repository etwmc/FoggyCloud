import * as http from 'http';
import * as url from 'url';

import * as msgQueue from "../dataMovement/messageQueue"

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
        case '/queue': {
        //Queue will process
            const lastVersion: number = requestURL.query['lastVer'];
            const offset: number = requestURL.query['offset'];
            if (lastVersion instanceof number && offset instanceof number) {
                msgQueue.queueRetrieve(userID, lastVersion, offset, function(data: NodeBuffer, newVersion: number, newOffset: number, errorType: string, error: string) {
                    if (error) {
                        //Did not found the delta file
                        response.statusCode = 500;
                    } else {
                        //Found the delta file
                        if (data.length > 0) {
                            //There is new messages
                            response.statusCode = 200;
                            response.end(
                                JSON.stringify({
                                    "msg": data,
                                    "ver": newVersion,
                                    "offset": newOffset
                                })
                            );
                        } else {
                            response.statusCode = 204;
                            response.end();
                        }
                    }
                });
            }
            break;
        }
        case '/snapshot': {
            //Vertification would be optional, here's it will just allow read/write snapshot without permission
            switch (requestURL.method) {
                case 'PUT':

            }
        }
        case '/init': {
            //Initial the account
            //Information needed: a user id hashed, and a user public certification
            switch (requestURL.method) {
                case 'POST':
                {
                    const userID: string = requestURL.query['uid'];
                    //Read the certification

                    break;
                }
            }
        }
    }
}

let server = http.createServer(handleRequest);

server.listen(surfacePort, function() {
    console.log("Start listening at %s", surfacePort);
})
