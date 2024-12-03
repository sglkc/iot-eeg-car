import Aedes from 'aedes'
import { createServer } from 'net'
import express from 'express'
import { Server } from 'socket.io'

const app = express()
const mqtt = new Aedes()
const api = createServer(app)
const ws = new Server(api)
const broker = createServer(mqtt.handle)

// jika ada client yang konek ke broker mqtt
mqtt.on('client', (client) => {
  console.log('client connected', client.id)
})

// jika ada pesan yang dipublish
mqtt.on('publish', (data) => {
  ws.emit(data.topic, data.payload)
})

// jika ada client yang konek ke websocket
ws.on('connection', (user) => {
  console.log('webscoket koneksi', user)
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

// port default mqtt broker
broker.listen(1883, () => {
  console.log('server started and listening on port ', 1883)
})

// port default api
api.listen(8080, () => {
  console.log('app listening')
})
