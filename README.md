# FoggyCloud
FoggyCloud is an experiment on building a fully encrypted cloud service, which balance both user privacy, simplicity of cloud storage, and the ability to perform data mining and machine learning to gather personal, and global insight. 

FoggyCloud consist of three components: 
#CloudVault
Cloud Vault is a C-based library, wihch transfer events to server in RSA 4096, and encrypt the whole database with CBC based AES256. It works for three stages: 
i. Transfer the events from user end to cloud, or receiver devices, using the public key of the receiver to encrypt, and append to the notification set
ii. When receive the a new event, perform the event indivdually on each device, on SQLite pages, and the modified pages are store separately under block mode similar to CBC. 
iii. When one of the devices is not operating, connect to Wi-Fi and power fully charged (suggested implemntation), a function merge the changes into a standalone, AES 256 CBC SQLite 3 databse, which will be transfer to the backend and used as the jumpstart when a new device shares the information. 
#DarkCloud
Dark Cloud as a simple HTML request responser work similar to modern web service, written in Node JS. It does not require major changes perofrm on the backend, as it works similar to basic cloud storage service.
#FoieGras
(https://github.com/etwmc/FoggyCloud/raw/master/FoieGras/logo.png)
Foie Gras is a backend system to receive a capture a set of vector, calculate a weighted average, and add it to the existing vector. In another word: When some devices sent you the weight updated vector acording to the dataset within the devices, Foie Gras will just consume them, and make your data model better. Thanks to the sum up process, and an one time hash generated for every model version that used for load balancing, it makes it extremely diffcult to trace the input vector from the data exchange, by anyone who try.   
This enables a distributed, large data set learning, outside of your data center, without specialized equiment, or storing users data as plaintext, so you get the imporvmenet you want, and users get their privacy they deserved.  
#etc
Foggy Cloud will also inclulde a data gathering library for operate and data mining service, without disclousing the identity of the user, or the entirety of the user data. 
