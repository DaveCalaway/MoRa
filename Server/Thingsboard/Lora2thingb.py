"""
 lora2thingb

 Estrae un oggetto JSON dalla struttura ricevuta da LoraSever.io
 e lo invia come telemetria a Thingsboard
"""

import sys, time
import logging
from logging.handlers import TimedRotatingFileHandler
import paho.mqtt.client as mqtt
import Queue
import json, urllib2, ssl

"""
 The MQTT Handler Class implements the interface towards MQTT Broker operation.
 It hides the details of the MQTT interaction to other components
 IMPORTANT NOTE: The MQTTHandler singleton must be used for this object in order to work
       The Paho callbacks need static methods and they can only use one single
       instance of the _MQTTHandler_class
"""
class Lora2thingb :

    # The logger facility: activate only if needed
    __logger = logging.getLogger('MQTTHandler')
    
    # The MQTT client instance
    # protocol 3 ensures that old Mosquitto 0.15 version is supported
    __mqttc = mqtt.Client(protocol=3) 

    # The message buffers for subscribed topics
    __mqtt_buffers = {}

    #Explicit Initializer
    def __init__(self):        
 
        # Logger setup: use the FileHandler only if need to troubleshoot
        hdlr = logging.StreamHandler(sys.stdout)
        formatter = logging.Formatter('%(asctime)s %(name)s %(levelname)s %(message)s')
        hdlr.setFormatter(formatter)
        Lora2thingb.__logger.addHandler(hdlr)
        Lora2thingb.__logger.setLevel(logging.DEBUG)
        self.mqtt_ip = 'localhost'

    """
    Initializes and connect to the MQTT Broker
    """
    def init_connect_mqtt(self, on_connect, on_message):
        Lora2thingb.__mqttc.on_connect = on_connect
        Lora2thingb.__mqttc.on_message = on_message

        #connect to the MQTT broker on, the controller
	#this connect is blocking.
	# No IP parsing at the moment - FUTURE
	broker_connected=False
	Lora2thingb.__logger.info('Trying to connect MQTT broker at ' + self.mqtt_ip)
	while not broker_connected:
            try:
                Lora2thingb.__mqttc.connect('localhost', 1884, 60)
                Lora2thingb.__logger.debug('Connected to MQTT broker on localhost')
                broker_connected = True
	    except:
		pass #do nothing
	    time.sleep(1)

	# Threaded call for MQTT handling
	Lora2thingb.__mqttc.loop_start()
	Lora2thingb.__logger.info('MQTT loop started')

    """
    Creates the subscription to a topic and the related queue
    to collect the messages returned on that topic
    """
    def add_subscription(self, topic):
        Lora2thingb.__mqttc.subscribe(topic)
        Lora2thingb.__logger.info('Added MQTT subscription on topic ' + topic)


    """
    Sends a message via MQTT on a given topic 
    """
    def send_mqtt_message(self, msg, topic): 
        Lora2thingb.__mqttc.publish(topic, msg)
        Lora2thingb.__logger.info('Published:' + msg + ' on topic ' + topic)

    """
    Stores a message received by the MQTT Client into the
    appropriate topic queue
    """
    def add_to_buffer(self, topic, message):
        Lora2thingb.__logger.info('Message received from MQTT server:' + topic + ' : ' + message)
        if not topic in Lora2thingb.__mqtt_buffers:
            # if there is no buffer queue for this topic, one is created
            # this makes the code more robust
            msgq = Queue.Queue()
            Lora2thingb.__mqtt_buffers[topic] = msgq

        Lora2thingb.__mqtt_buffers[topic].put(message)


    """
    Retrieves the first available message from MQTT for a given topic
    Returns the messsage to forward
    """
    def receive_mqtt_msg(self, topic):
        new_msg = None
        if topic in Lora2thingb.__mqtt_buffers:
            msgq = Lora2thingb.__mqtt_buffers[topic]
            if not msgq.empty():
                new_msg = msgq.get_nowait()
                Lora2thingb.__logger.info('Popped up from topic '+ topic +' queue:' + new_msg)

        return new_msg

### Lora2thingb class definition ends here

# Inizio programma

mqtt_handler = Lora2thingb()
#The callback for when the client receives a CONNACK response from the server.
def _on_mqtt_connect(client, userdata, flags, rc):
    # TODO: May be needed to handle reconnection of all subscribed topics
    #       in case of reconnection...?
    pass

#The callback for when a PUBLISH message is received from the server.
def _on_mqtt_message(client, userdata, msg):
    mqtt_handler.add_to_buffer(msg.topic, str(msg.payload))

###### End MQTT Callbacks ########
mqtt_handler.init_connect_mqtt(_on_mqtt_connect, _on_mqtt_message)

mqtt_handler.add_subscription('application/1/#')

while True:
    time.sleep(0.5)
    msg = mqtt_handler.receive_mqtt_msg('application/1/device/0000000000000001/rx')
    if msg != None:
        print msg
        #parsing dell'oggetto JSON contenuto nel messaggio
        oggetto = json.loads(msg)
        misure = oggetto["object"]
        v_temp = misure["temp"]
        v_hum = misure["hum"]
        #Invio a Thingsboard
        recvalues = [{"temp": v_temp}, {"hum": v_hum}]
        resp = ''
        print json.dumps(recvalues)
        try:
            req = urllib2.Request("http://localhost:8080/api/v1/4tH3W2PEfAyjlTSRExsi/telemetry", data=json.dumps(recvalues), headers = {'Content-Type': 'application/json'})
            #resp = urllib2.urlopen(req, context=self.ssl_ctx)
            resp = urllib2.urlopen(req)
        except:
            print 'JSON Record post failed. Response was: ' + resp
            print 'Exception info:' + str(sys.exc_info()[0])
