#! /usr/bin/python
 
import os
from gps import *
from time import *
import time
import threading
from BaseHTTPServer import BaseHTTPRequestHandler,HTTPServer
import json
from mylog import myLog
from datetime import datetime

PORT_NUMBER = 9001

main_directory = "/home/camera/vte"
log_directory = main_directory + "/logs"
data_directory = main_directory + "/data"
gpslog_directory = data_directory + "/gps/"
logfile_out = log_directory + "/status.log"

gpsd = None   #seting the global variable

#This class will handles any incoming request from
#the browser

class gpsHandler(BaseHTTPRequestHandler):
	
    #Handler for the GET requests
    def do_GET(self):
	
	#This function handles sending the json file to the client upon request
	
        self.send_response(200) #HTTP protocol for successful requests code 200 in response to GET
        self.send_header('Content-type','text/html') #Writes a specific HTTP header to the output stream
        self.end_headers() #Sends a blank line, indicating the end of the HTTP headers in the response
        # Send the json message
        message = json.dumps({ #Sets up json file so data is parsed based on the string preceding the variables
                        'Latitude'  : curr_lat,
                        'Longitude' : curr_long,
                        'Time'      : curr_time,
                        'Speed'     : curr_speed,
                        'Heading'   : curr_heading,
                        'ErrorEps'  : curr_eps,
                        'ErrorEpx'  : curr_epx,
                        'ErrorEpv'  : curr_epv,
                        'ErrorEpt'  : curr_ept
                        })
        self.wfile.write(message) #Sends the json message to client
        return

    def log_message(self, format, *args): 
        return

class GpsPoller(threading.Thread):
    
  def __init__(self):
  
  #Ask about this function, unsure about daemons etc.
  
    threading.Thread.__init__(self) #Spawn a new thread (multithreading)
    global gpsd #Make gpsd global, I assume this gpsd is now a global object
    gpsd = gps(mode=WATCH_ENABLE) #Starts the stream of info for gps data
    self.current_value = None 
    self.running = True #Thread running is TRUE
    self.daemon = True 
 
  def run(self):
  #This function keeps the gpsd running and keeps grabbing info from the gps module (I think)
    global gpsd
    while gpsp.running:
      gpsd.next() #this will continue to loop and grab EACH set of gpsd info to clear the buffer
 
if __name__ == '__main__':
  gpsp = GpsPoller()   # Create the GPS Poller Thread
  
  try:

        log = open(logfile_out, "a", 1) # non blocking

        gpslog = myLog(gpslog_directory,'gps','csv')

        gpsp.start() # start it up

        #
        #  Start up simple HTTP Server to handle requests for radar information
        #
        server = HTTPServer(('', PORT_NUMBER), gpsHandler)

        t=threading.Thread(target=server.serve_forever)
        t.daemon = True
        t.start()

        print 'Started httpserver on port ' , PORT_NUMBER
        print "Reading Serial Port..."

        while True:
            curr_lat = gpsd.fix.latitude
            curr_long = gpsd.fix.longitude
            curr_time = gpsd.utc
            curr_eps = gpsd.fix.eps
            curr_epx = gpsd.fix.epx
            curr_epv = gpsd.fix.epv
            curr_ept = gpsd.fix.ept
            curr_speed = round(gpsd.fix.speed * 2.23694)
            curr_heading = gpsd.fix.track

            current_timestamp = datetime.now().strftime('%Y-%m-%d %H;%M:%S')

            gpslog.write_log(str(current_timestamp) + ', ' + \
                             str(curr_time) + ', ' + \
                             str(curr_lat) + ', ' + \
                             str(curr_long) + ', ' + \
                             str(curr_speed) + ', ' + \
                             str(curr_heading) + ', ' + \
                             str(curr_eps) + ', ' + \
                             str(curr_epx) + ', ' + \
                             str(curr_epv) + ', ' + \
                             str(curr_ept) + '\n')
     
            time.sleep(1) #set to whatever
 
  except (KeyboardInterrupt, SystemExit): #when you press ctrl+c
    print "\nKilling Thread..."
    gpsp.running = False
    gpsp.join() # wait for the thread to finish what it's doing
  print "Done.\nExiting."

