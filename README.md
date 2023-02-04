## Final Term
# File server
- File server will broadcast message in form:
    * IPBroadcast {IP address of computer server} {Name of computer server} {Port of process share file}
- In process share file
    * To upload file
        + Send to server command: Upload {file name or path with file name (extension is required)}
        + Send data of file
        + Send command Done to end
    * To download file from server
        + Send command: Download {file name}
        + Server will send data to client
        + After sent all data, server send "END" to notice that downloading is done
    * To view list of file shared in server
        + Send command: List
        + Server will send list file can be shared now in server
        + If no file can be shared, server return "Empty"