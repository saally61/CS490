import RPi.GPIO as GPIO
from picamera import PiCamera
import time
import smtplib
import datetime
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText
from email.mime.base import MIMEBase
from email import encoders
import os
import _thread
import mysql.connector
import io
import picamera
import logging
import socketserver
from threading import Condition
from http import server
import requests
import json
import base64
from PIL import Image, ImageDraw, ImageFont
import threading


fromaddr = "email"
toaddr = "email"
GPIO.setmode(GPIO.BCM) #set up board
GPIO.setwarnings(False) # ignore warnings
GPIO.setup(18,GPIO.OUT) #Set up relay GPIO, output
GPIO.setup(4,GPIO.IN) # setup motion sensor GPIO, input
KEY = 'KEY'  # Replace with a valid Subscription Key here.
BASE_URL = 'https://eastus.api.cognitive.microsoft.com/face/v1.0/detect'  # Replace with your regional Base UR

headers = {
     'Content-Type': 'application/octet-stream',
     'Ocp-Apim-Subscription-Key': KEY,
}

params = {
    'returnFaceId': 'true',
    'returnFaceLandmarks': 'false',
    'returnFaceAttributes': 'age,gender,glasses,smile'

}

#check the database for an update
#kill server when db says so
def checkdb():
    while(True):
        #check every second
        time.sleep(1)
        mydb = mysql.connector.connect(
          host="localhost",
          user="pi",
          passwd="PASSWORD",
          database="STAT"
        )
        mycursor = mydb.cursor()
        mycursor.execute("SELECT * FROM Request")

        myresult = mycursor.fetchall()
        x = next(iter(myresult), None)
        #used to print status
            #print(x)
        if(x[1]==0):
            server.shutdown()
            print("Shuting down server")
            return
            
#Gets a file and checks for faces        
def get_face(name):
    global succ_img
    file_name = '/home/pi/Hackathon_Video/' + name+'.jpg'
    with open(file_name, 'rb') as f:
        img_data = f.read()
    try:
        response = requests.post(BASE_URL,
                                 data=img_data, 
                                 headers=headers,
                                 params=params)
    
        parsed = response.json()
        print(parsed)
        print("Found :"+ str(len(parsed)) +" faces in the image: "+ file_name)
        if(len(parsed)==0 or succ_img >=2):
            os.remove(file_name)
            return
        else:
            succ_img +=1
        source_img = Image.open(file_name)
        for facex in parsed:
            #print the face data *debuging*
            #print(facex)
            # display the image analysis data
            width = facex['faceRectangle']['width']
            top = facex['faceRectangle']['top']
            height = facex['faceRectangle']['height']
            left = facex['faceRectangle']['left']
            #print (top,left , width, height )#parsed[0]['faceRectangle'])
            fnt = ImageFont.truetype('Pillow/Tests/fonts/FreeMono.ttf', 14)
            draw = ImageDraw.Draw(source_img)
            draw.rectangle(((left, top), (width+left, height+top)), outline="red")
            draw.text((left, top+height),facex['faceAttributes']['gender'] + "," +str(facex['faceAttributes']['age']) + "\n" +str(facex['faceAttributes']['glasses']) + "\n"+
                   "smile:"+ str(facex['faceAttributes']['smile']),(255,255,255),fnt)
        
        os.remove(file_name)
        #used to check file name *debugging*
        #print(file_name)
        
        source_img.save("/home/pi/Hackathon_Video/"+name+"result.jpeg", "JPEG")
        
        #used to check the new image name
        #print("giving : " +file_name+"result")
        
        send_img(name+"result")
        
    except Exception as e:
        os.remove(file_name)
        print("Effor : " + e)
        
##html for the page
PAGE= """
<html>
<head>
    
	<title>Doorman</title>
    <style >
     * {
       margin: 0; 
       padding-top: 0px;
     }
   	.button {
  		background-color: #4CAF50;
 		border: none;
  		color: white;
  		padding: 20px 32px;
  		text-align: center;
  		text-decoration: none;
  		display: inline-block;
  		font-size: 16px;
  		margin: 20px 2px;
  		cursor: pointer;
        border-radius: 12px;
		}
	.button:hover {
  			box-shadow: 0 12px 16px 0 rgba(0,0,0,0.24),0 17px 50px 0 							rgba(0,0,0,0.19);
            }

     
		h1.ex1 {
            font-size:60px;
			}
        h3.ex2 {
            font-size:20px;
			}




.hero-image {
  background-image: url("https://cdn-images-1.medium.com/max/1254/1*FlR4UCK0YosNvZgp_feReg.jpeg");
  background-color: #cccccc;
  height: 900px;
  background-position: center;
  background-repeat: no-repeat;
  background-size: cover;
  position: relative;
}

.hero-text {
  text-align: right;
  position: absolute;
  top: 50%;
  left: 65%;
  transform: translate(-50%, -50%);
  color: white;
}




</style>
</head>




<body>


    <div class="hero-image" >
  <div class="hero-text">
    <h1 style="font-size:70px">AI Doorman</h1>
    <h1 style="text-align: center;">Live video</h1>
    <img src="stream.mjpg" width="640" height="480" alt="centered image" />
    <button class="button" onclick="location='http://192.168.1.236/return.php'">Stop Video</button>
  </div>
</div>
    
  
</body>
</html>"""

##streaming class
class StreamingOutput(object):
    def __init__(self):
        self.frame = None
        self.buffer = io.BytesIO()
        self.condition = Condition()

    def write(self, buf):
        if buf.startswith(b'\xff\xd8'):
            # New frame, copy the existing buffer's content and notify all
            # clients it's available
            self.buffer.truncate()
            with self.condition:
                self.frame = self.buffer.getvalue()
                self.condition.notify_all()
            self.buffer.seek(0)
        return self.buffer.write(buf)

##Streaming hanler
class StreamingHandler(server.BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/':
            self.send_response(301)
            self.send_header('Location', '/index.html')
            self.end_headers()
        elif self.path == '/index.html':
            
            content = PAGE.encode('utf-8')
            self.send_response(200)
            self.send_header('Content-Type', 'text/html')
            self.send_header('Content-Length', len(content))
            self.end_headers()
            self.wfile.write(content)
        elif self.path == '/stream.mjpg':
            self.send_response(200)
            self.send_header('Age', 0)
            self.send_header('Cache-Control', 'no-cache, private')
            self.send_header('Pragma', 'no-cache')
            self.send_header('Content-Type', 'multipart/x-mixed-replace; boundary=FRAME')
            self.end_headers()
            try:
                while True:
                    with output.condition:
                        output.condition.wait()
                        frame = output.frame
                    self.wfile.write(b'--FRAME\r\n')
                    self.send_header('Content-Type', 'image/jpeg')
                    self.send_header('Content-Length', len(frame))
                    self.end_headers()
                    self.wfile.write(frame)
                    self.wfile.write(b'\r\n')
            except Exception as e:
                logging.warning(
                    'Removed streaming client %s: %s',
                    self.client_address, str(e))
        else:
            self.send_error(404)
            self.end_headers()
##streaming server class
class StreamingServer(socketserver.ThreadingMixIn, server.HTTPServer):
    allow_reuse_address = True
    daemon_threads = True
##simple light check. 
def lightOn():
    print ("Light on")
    GPIO.output(18,GPIO.HIGH)
    time.sleep(1)
    print ("Light off")
    GPIO.output(18,GPIO.LOW)
## send file through email. 
def send_img(file_name):
    fromaddr = "email"
    toaddr = "email"
     
    msg = MIMEMultipart()
     
    msg['From'] = fromaddr
    msg['To'] = toaddr
    msg['Subject'] = "Motion at " + file_name 
     
    body = "Motion was detected"
     
    msg.attach(MIMEText(body, 'plain'))
     
    filename = file_name + '.jpeg'
    #used to print the file name *debuging *
    #print("Updated to: " +filename)
    attachment = open("/home/pi/Hackathon_Video/" + filename, "rb")
     
    part = MIMEBase('application', 'octet-stream')
    part.set_payload((attachment).read())
    encoders.encode_base64(part)
    part.add_header('Content-Disposition', "attachment; filename= %s" % filename)
     
    msg.attach(part)
     
    server = smtplib.SMTP('smtp.gmail.com', 587)
    server.starttls()
    server.login(fromaddr, "password")
    text = msg.as_string()
    server.sendmail(fromaddr, toaddr, text)
    server.quit()
    os.remove("/home/pi/Hackathon_Video/" + file_name + '.jpeg')

recording = False
lightOn() #
camera = PiCamera() #Set up camera
camera.resolution = (640, 480)
time.sleep(1) #let everything set up
pic_name = ""
address = ('',8000)
server = StreamingServer(address, StreamingHandler)
print("Looking for motion and waiting for Commands through the Database")
succ_img = 0
while True:
    time.sleep(1)
    mydb = mysql.connector.connect(
          host="localhost",
          user="pi",
          passwd="PASSWORD",
          database="STAT"
        )
        
    mycursor = mydb.cursor()
    mycursor.execute("SELECT * FROM Request")

    myresult = mycursor.fetchall()
    x = next(iter(myresult), None)
    #print(x)
    if(x[1]==0):
        if(GPIO.input(4)): #if there is motion
            if(not recording):  #check if we are
                print ("Light on")
                GPIO.output(18,GPIO.HIGH)
                camera.start_preview()
                recording = True
                
            else:
                if(succ_img <1):
                    time.sleep(2)
                    date_time = datetime.datetime.now()
                    pic_name = date_time.strftime("%m-%d_%H:%M:%S")
                    f = '/home/pi/Hackathon_Video/' + pic_name+'.jpg'
                    camera.capture(f)
                    t = threading.Thread(target=get_face, args=(pic_name,))
                    t.start()
        else:
            temp = False
            if(recording):
                succ_img = 0
                GPIO.output(18,GPIO.LOW)
                camera.stop_preview()   
                print ("Light off")
                recording = False
    else:
        GPIO.output(18,GPIO.HIGH)
        output = StreamingOutput()
        camera.start_recording(output, format='mjpeg')
        try:
            t = threading.Thread(target=checkdb)
            t.start()
            print("live video server active")
            server.serve_forever()
            print("Server closed")
        finally:
            camera.stop_recording()

