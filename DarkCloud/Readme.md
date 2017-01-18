DarkCloud is the backend of the FoggyCloud project. It handles the logistic of message passing (and pushing when you implement), public key sharing and encrypted snapshot storage. 

Run: node ./build/server/interface/dbSurface.js

There is 5 RESTful functions: 

#/init
It initialize the storage space, and storing the one time public key setting
Method: POST
Parameter: none
Body: the public key of the users
Return: User ID in UUID form

#/key
It covers the user public key, which is used to wrap and unwrap message key. 
Wrapping scheme: if the sender and receiver are the same (self looping message), the key is wrap once using the public key
If the sender and receiver are different, the key is first wrap with sender private key, then with receiver public key. 
Method: GET
Parameter: uid: User ID in UUID form
Body: nothing
Return: Body is the public key

#/queue
FoggyCloud has one central point for determining serialization: the backend. While the content of the changes are shared out of order, out of sync, and by various way (from the device conduct the changes, the devices that receive the changes, or the backend), the backend maintain the sole copy of the message order. 
Method: POST
Parameter: 
1. uid: Sender User ID in UUID form
2. tuid: Target User ID in UUID form
Header: body-signature: a hash of body, wrap under private key, can could check using user public key
Body: First 32 bytes as AES-256 key wrapped, then the message body wrapped in AES-GCM
Return: 
None

Method: GET
Parameter: 
1. uid: User ID in UUID form
2. lastVer: last notification version (currently version is 0. It will be used only when the message order log over 2^53-1, which will takes 80 trillion messages. )
3. Offsest: last fetch offset
Body: None
Return: 
Header: Version: new version (remain 0), Offset: new file offset
Body: a series of JSON dictionary

#/message
It fetches the actual message in case the device did not get notify by nearby devices
Method: GET
Parameter:
1. uid: User ID in UUID form
2. mid: Message ID in UUID form
Body: None
Return: Body: the message content

#/snapshot
Snapshot is a term in FoggyCloud describe the database pause for operations (preferably under not functioning period, like 2am in user timezone), and reconstruct and merge the database file into one encrypted file. The file is then uploaded to the backend, so it can used as a rapid catch up version. 
Method: PUT
Parameter: 
1. uid: User ID in UUID form
2. ver: Version of message the database is based on
3. offset: Offset of the message the database is based on
Body: The snapshot
Return: None

Method: GET
Parameter: uid: User ID in UUID form
Body: None
Return: 
Header: Offset, Version
Body: Snapshot