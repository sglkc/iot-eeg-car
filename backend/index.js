import Aedes from 'aedes'
import { createServer } from 'net'
import express from 'express'

const api = express()
const aedes = new Aedes()
const broker = createServer(aedes.handle)

api.use(express.query())

aedes.on('client', (client) => {
  console.log('client connected', client.id)
})

broker.listen(1883, () => {
  console.log('server started and listening on port ', 1883)
})

api.get('/send', (req, res) => {
  const { text } = req.query

  if (!text) return res.status(400).send('text query missing')

  aedes.publish({
    topic: 'first/test',
    payload: text
  })

  return res.send('success')
})

api.listen(8080, () => {
  console.log('api listening')
})
