To compile:
make

To run proxy server:
./webproxy <PORT Number> <Cache timeout>

./webproxy 10001 100


Testing Instructions 
Clear the Mozilla cache and local proxy server cache 


For Testing with http browser (Mozilla Firefox)
1)http://www.umich.edu/~chemh215/W09HTML/SSG4/ssg6/html/Website/DREAMWEAVERPGS/first.html
2)www.google.com 
3)other (http (not https))websites

For Testing with telnet
telnet 127.0.0.1 10001
GET http://www.google.com HTTP/1.1


Implementation of Code:


1) Proxy Implementation - 
 The webproxy parses the request from client.
 Checks if present in cache
 If exists 
      displays CACHE MEMORY FOUND and populates information from cache to client
 else
       displays NO CACHE FOUND and passes the appropriate request to server
 Sends the requested information back to client and parallely stores it on cache memory



2) Caching and Timeout implementation 

-Caching
For caching and searching file , MD5 hashing is been used

-Timeout
current time - file modified time  < timeout - file is fetched from cache 
else
	File fetched from Server (and replaces the old file if present)

- Hostname and IP address Cache implementation:
	Hostnames are checked for corresponding IP address from a file if already resolved. Else, DNS query is done and the hostname and its IP address is stored in the file.

-Blocked sites:
Everytime a website and its IP address are checked for in the Blocked Sites file. If the website or the IP address are present in the file, 403 Forbidden message is returned to the client.

3) Prefetching Implementation 
The links present in the file from server are extracted and passed onto server
Server responds appropriately with the request and is stored in the cache 




