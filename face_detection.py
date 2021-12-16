import cv2, serial, time, math

time.sleep(1) # wait to make sure the USB connection has time time to connect correctly

faceCascade = cv2.CascadeClassifier(cv2.data.haarcascades + 'haarcascade_frontalface_default.xml') # Sets the dataset to the built in faces xml
video_capture = cv2.VideoCapture(1) # change to 1 when the video capture device is the second camera

try:
    ser = serial.Serial('COM7', 115200)  # Establish the connection on a specific port
    time.sleep(3)
    if ser.isOpen():
        print("port is opened!")
    
except IOError: # if port is already opened, close it and open it again and print message
    ser.close()
    ser.open()
    print ("port was already open, was closed and opened again!")

while True:
    # Capture frame-by-frame
    ret, frame = video_capture.read()

    gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

    faces = faceCascade.detectMultiScale(
        gray,
        scaleFactor=1.1,
        minNeighbors=6, # this is basically the sensitivity - it is the number of features needed to clasify something as a face (was 5 to start with)
        minSize=(30, 30),
        flags=cv2.CASCADE_SCALE_IMAGE
    )

    # Draw a rectangle around the faces and a point in the center of them
    for (x, y, w, h) in faces:
        #cv2.rectangle(frame, (x, y), (x + w, y + h), (0, 255, 0), 2)
        pointX = int(x + (w / 2))
        pointY = int(y + (h / 2))

        goalY = 240
        errorX = int(pointX - 320)
        errorY = int(pointY - goalY -120) # FIXED VERTICAL OFFSET - IDEALLY, THIS WOULD ADJUST DEPENDING ON THE DISTANCE FROM THE TARGET
        distance = int(h * 10)  # distance from the camera to a face (in cm)
        
        msgToArduinoString = f'&{round(errorX, 2)}:{round(errorY, 2)}'
        while len(msgToArduinoString) < 10:
            msgToArduinoString = msgToArduinoString + "."

        cv2.circle(frame, (pointX, pointY), 5, (0, 0, 255), -1)

        #cv2.circle(frame, (320, 240), 5, (255, 0, 0), -1) # Draws dot at center of frame

        ################## This shows the error as an updating line - is commented out to allow the arduino message to be printed ##################
        #positionStr = 'X: ' + str(x).rjust(4) + ' Y: ' + str(y).rjust(4) + ' errorX: ' + str(errorX).rjust(4) + ' errorY: ' + str(errorY).rjust(4)
        #print(positionStr, end='')
        #print('\b' * len(positionStr), end='', flush=True)
        
        #print(msgToArduino)
        #ser.write(bytes(msgToArduino, encoding='utf8'))  # converts message to bytes and sends it over serial

        try:
            ser.write(bytes(msgToArduinoString, encoding='utf8'))  # converts message to bytes and sends it over serial
            print(ser.readline())
        except Exception as e: 
            print(e)

    # Display the resulting frame
    cv2.imshow('Video', frame)

    if cv2.waitKey(1) & 0xFF == ord('q'):
        break

# When everything is done, release the capture
video_capture.release()
cv2.destroyAllWindows()