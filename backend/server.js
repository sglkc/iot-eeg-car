import net from 'node:net'
import http from 'node:http'
import Aedes from 'aedes'
import express from 'express'
import { Server } from 'socket.io'
import { MongoClient } from 'mongodb'

const app = express()
const mqtt = new Aedes()
const api = http.createServer(app)
const ws = new Server(api)
const broker = net.createServer(mqtt.handle)
const mongo = new MongoClient('mongodb://localhost:27017')
const db = mongo.db('eeg-project')

// jika ada client yang konek ke broker mqtt
mqtt.on('client', (client) => {
  console.log('mqtt client connected', client.id)
})

// semua pesan yang dipublish ke mqtt dikirim juga ke websocket
mqtt.on('publish', (data) => {
  ws.emit(data.topic, data.payload)
})

// jika ada client yang konek ke websocket
ws.on('connection', (socket) => {
  console.log('websocket client connected', socket.id)

  // setiap ada pesan websocket, kriim ulang ke mqtt
  socket.onAny((topic, payload) => {
    mqtt.publish({ topic, payload })
  })
})

app.get('/', (_, res) => {
  return res.sendFile(import.meta.dirname + '/index.html')
})

// endpoint api
app.get('/send', (req, res) => {
  const { text } = req.query

  if (!text) return res.status(400).send('text query missing')

  mqtt.publish({
    topic: 'first/test',
    payload: text
  })

  return res.send('success')
})

mongo.connect().then(() => {
  console.log('mongo db connected')
})

// port default mqtt broker
broker.listen(1883, () => {
  console.log('mqtt broker on port', 1883)
})

// port default api
api.listen(8080, () => {
  console.log('api & websocket on port', 8080)
})
