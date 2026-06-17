import paho.mqtt.client as mqtt
from struct import pack
from random import randint
from time import sleep
 
SENSOR_ID = 5000
 
# topicos providos por este sensor
tt = "sensor/%s/temperatura" % (SENSOR_ID)
ut = "sensor/%s/umidade" % (SENSOR_ID)
 
# cria um identificador baseado no id do sensor
client = mqtt.Client(client_id = 'SENSOR:%d' % (SENSOR_ID),
                     protocol = mqtt.MQTTv31)
# conecta no broker
client.connect("127.0.0.1", 1883)
 
while True:
    # gera um valor de temperartura aleatório
    t = randint(0,30)
    # codificando o payload como big endian, 2 bytes
    payload = pack(">H",t)
    # envia a publicação
    client.publish(tt,payload,qos=0)
    
    print (tt + "/" + str(t))
    
    # gera um valor de umidade aleatório
    u = randint(0,60)
    # codificando o payload como big endian, 2 bytes
    
    payload = pack(">H",u)
    
    # envia a publicação
    client.publish(ut,payload,qos=0)
    print (ut + "/" + str(u))
    
    sleep(10)
