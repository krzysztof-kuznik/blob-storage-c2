# Azure Blob Storage C2
Simple proof of concept utilizing [Azure Blob Storage](https://azure.microsoft.com/en-us/products/storage/blobs) as a method of C2 communication.

## How it works
Binary uses native azure libraries to communicate with blob storage container account to read specific file (`command.txt` by default).
Then it executes whatever was inside this file as a command on the system. Result of this command is uploaded to another file on storage container (`results.txt` by default).

## How do I use it ?
Well first you gotta have Azure Blob Storage Account and then you can input those details in the code:
```
static const char* AZURE_STORAGE_CONNECTION_STRING = "DefaultEndpointsProtocol=https;AccountName=nameOfYourStorageAccount;AccountKey=pasteYourStorageAccountKeyHere;EndpointSuffix=core.windows.net";
std::string containerName = "cnc-test";
std::string resultsFileName = "results.txt";
std::string commandFileName = "command.txt";
```
Then you can edit the file directly through web browser or [API](https://learn.microsoft.com/en-us/rest/api/storageservices/blob-service-rest-api) or you can write a cool python cmd line tool.

## Cool but why ?
Usage of native MS services makes this method less prone to be detected. Also - beaconing blob storage will rarely be detected on network level or will be omitted by analysts as a false-positive.
