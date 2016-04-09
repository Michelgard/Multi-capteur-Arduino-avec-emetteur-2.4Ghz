#!/usr/bin/env python
# -*- coding: utf-8 -*-

from nrf24 import NRF24
import time
from time import gmtime, strftime
import MySQLdb
import xml.dom.minidom
import sys

verbose = 0
if len(sys.argv) > 1:
    if sys.argv[1] == "-v":
        verbose = 1
    else:
        print "Argument non reconnu !  -v pour verbose"
        sys.exit(0) 

"""import données connexion SGL d'un fichier config en XML"""
tree = xml.dom.minidom.parse("/home/pi/nrf24pihub/configSQL.xml")
valeurListe = tree.getElementsByTagName("SQL")

"""Variable de connexion R24"""
pipes = [[0xf0, 0xf0, 0xf0, 0xf0, 0xe1], [0xf0, 0xf0, 0xf0, 0xf0, 0xd2]]

""" init variable"""
temper = ''
humid = ''
temper2 =''
press = ''
temperExt = '0.0'

"""Initialisation connexion R24"""
radio = NRF24()
radio.begin(0, 0,25,18) #set gpio 25 as CE pin
radio.setRetries(15,15)
radio.setPayloadSize(52)
radio.setChannel(0x4c)
radio.setDataRate(NRF24.BR_250KBPS)
radio.setPALevel(NRF24.PA_MAX)
radio.setAutoAck(1)
radio.openWritingPipe(pipes[0])
radio.openReadingPipe(1, pipes[1])

radio.startListening()
"""fin initialisation R24"""

"""Fonction pour extraire de la variable receptionnée les différentes valeurs"""
def extract(raw_string, start_marker, end_marker):
    start = raw_string.index(start_marker) + len(start_marker)
    end = raw_string.index(end_marker, start)
    return raw_string[start:end]

if verbose:
    print
    print ("Attente réception du capteur")
    print

"""Boucle infinie de réception des données"""
while True:
    pipe = [0]
    """Si pas de réception on attends ..."""
    tour = 1
    while tour < 3:
        while not radio.available(pipe, True):
            time.sleep(1000/1000000.0)
        recv_buffer = []
        radio.read(recv_buffer) #Les données réceptionnées sont mise dans la variable
        out = ''.join(chr(i) for i in recv_buffer)#création d'une variable blobage avec les valeurs du tableu buffer

        """Création de la date et heure en francais et en SQL"""
        now = time.localtime(time.time())  
        year, month, day, hour, minute, second, weekday, yearday, daylight = now
        dateheure = "%02d/%02d/%04d" % (day, month, year) + "  " + "%02d:%02d:%02d" % (hour, minute, second)
        dateheureSQL = "%04d-%02d-%02d" % (year, month, day) + " " + "%02d:%02d:%02d" % (hour, minute, second)
        
        """"La variable out est décortiquée avec les données de numéro de capteur, température, humidité, tension pile """
        if out[2] == '1':
            temper=extract(out,'T','T')
            humid=extract(out,'H','H')
        if out[2] == '2':
            temper2=extract(out,'M','M')
            press=extract(out,'P','P')
            if out[21] =='E':
                temperExt=extract(out,'E','E')
        tour = tour + 1

    """affichage des données recues pour es teste"""
    if verbose:
        print (dateheure)
    if verbose:
        print ("La temppérature   : " + temper + "°C")
        print ("Niveau d'humidité : " + humid + "%")
        print ("La température 2 : " + temper2 + "°C")
        print ("La pression : " + press + " ")
        print ("La température Ext : " + temperExt + "°C")
        print
 
    """Connexion et insertion de la données dans la base"""
    for valeur in valeurListe:
        #connexion  à la base de données
        db = MySQLdb.connect(valeur.attributes['ip'].value, valeur.attributes['login'].value, valeur.attributes['mdp'].value, valeur.attributes['dbase'].value)
    dbSQL = db.cursor()
    sql = "INSERT INTO `Capteurs_Multi`(`Date_Heure`, `Temperature`, `Humidite`, `Temperature2`, `Pression`, `TemperatureExterieur`)\
    VALUES ('" + dateheureSQL + "', " + temper + ", " + humid + ", " + temper2 + ", " + press + ", " + temperExt + ")"
    
    if verbose:
        print sql
    dbSQL.execute(sql)
    db.commit()

    db.close()