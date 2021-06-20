# -*-coding:utf-8-*-

# VR ===> i2c adc ==> raspi zero
# ADC で変換した可変抵抗の抵抗値をUDP で 同一AP に接続したM5stack CORE2に表示する


from socket import socket, AF_INET, SOCK_DGRAM
import smbus
import time

i2c = smbus.SMbus(1) # MCP3425 を接続したi2cポートのアドレス

HOST = ""
PORT = 5000
ADDR = "192.168.***.***" # M5stack のIPアドレス

s = socket(AF_INET, SOCK_DGRAM)

while True:
    

s.close()