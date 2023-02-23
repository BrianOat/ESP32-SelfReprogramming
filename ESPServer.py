# Python 3 server example
from http.server import BaseHTTPRequestHandler, HTTPServer
import time
import socket
import os
hostName = socket.gethostbyname(socket.gethostname())#gets IP of device
serverPort = 8000
#Function that will take packets 
def send_packets(filename: str):
    packets = []
    with open(filename, "rb") as file:
        while True:
            buffer = file.read(2000)
            if not buffer:
                break
            packet = buffer
            packets.append(packet)
    return packets

# # Print out the contents of the list
# for i in range(len(packets)):
#     print(f"Packet {i}: {packets[i]}")


packet_counter = 0
packets = []
class MyServer(BaseHTTPRequestHandler):
    global packets, packet_counter
    def do_GET(self):
        self.send_response(200)
        self.send_header("Content-type", "text/html")
        self.end_headers()
        print("PATH:"+ self.path)
        #Update part
        if self.path=="/Updates.json":
            print("WORKS")
            with(open("Updates.json", "r")) as f:
                self.wfile.write(bytes(f.read(), "utf-8"))
            #Checks if a binary file exists
            try:
                #change with proper way to verify file
                with(open(self.path, "rb")) as f:
                    pass
                packets = send_packets()
            except Excepton:
                self.wfile.write(bytes("INVALID REQUEST", "utf-8"))
        #go to /packets and ping that many times 
        elif self.path == "/packets" and packets !=[]:
            self.wfile.write(packets[packet_counter])
            packet_counter += 1
            if packet_counter == len(packets):
                packet_counter = 0
                packets = []

if __name__ == "__main__":        
    webServer = HTTPServer((hostName, serverPort), MyServer)
    print("Server started http://%s:%s" % (hostName, serverPort))

    try:
        webServer.serve_forever()
    except KeyboardInterrupt:
        pass

    webServer.server_close()
    print("Server stopped.")