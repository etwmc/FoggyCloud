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
#etc
Foggy Cloud will also inclulde a data gathering library for operate machine learning and data mining service, without disclousing the identity of the user, or the entirety of the user data. 
