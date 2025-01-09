import net from 'node:net'
import http from 'node:http'
import Aedes from 'aedes'
import express from 'express'
import cors from 'cors'
import { Server } from 'socket.io'
import { MongoClient } from 'mongodb'

const DB = process.env.DB

const app = express()
const mqtt = new Aedes()
const api = http.createServer(app)
const ws = new Server(api)
const broker = net.createServer(mqtt.handle)
const mongo = new MongoClient('mongodb://localhost:27017')
const db = mongo.db(DB ?? 'eeg-project')

const log = (...text) => {
  const date = new Date()
  const time = date.toLocaleTimeString('ru')
  const ms = date.getMilliseconds().toString().padStart(3, '0')
  console.log(time + '.' + ms, '|', ...text)
}

app.use(cors())

// jika ada client yang konek ke broker mqtt
mqtt.on('client', (client) => {
  log('mqtt client connected', client.id)
})

// semua pesan yang dipublish ke mqtt dikirim juga ke websocket
mqtt.on('publish', (data) => ws.emit(data.topic, data.payload))

// jika ada client yang konek ke websocket
ws.on('connection', (socket) => {
  log('websocket client connected', socket.id)

  socket.on('disconnect', () => {
    log('disconnected', socket.id)
  })

  // setiap ada pesan websocket, kriim ulang ke mqtt
  // tambah ke database jika ada sinyal eeg atau command
  socket.onAny((topic, payload) => {
    const type = typeof payload

    mqtt.publish({ topic, payload })

    log(new Date().toISOString(), topic, ':', type !== 'string' ? type : payload)

    if (DB)
    switch (topic) {
      case 'esp32/commands':
        db.collection('commands').insertOne({
          command: payload,
          time: new Date().toISOString()
        })
        break
      case 'eeg':
        db.collection('eeg_signals').insertOne({ ...payload })
        break
    }
  })
})

app.use('/', express.static(import.meta.dirname + '/ui'))

// endpoint api
app.get('/send', (req, res) => {
  if (!req.is('application/json')) {
    res.status(400).send('body must be json');
    return
  }

  ws.emit('data', req.body)
  res.status(200).sebd('success')
})

if (DB)
mongo.connect()
  .then(() => log('mongo db connected'))
  .catch((err) => log('mongo db error!', err))

// port default mqtt broker
broker.listen(1883, () => {
  log('mqtt broker on port', 1883)
})

// port default api
api.listen(8080, () => {
  log('api & websocket on port', 8080)
})
