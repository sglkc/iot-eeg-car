import path from 'node:path';
import { createServer } from 'node:http';
import { Server } from 'socket.io';
import express from 'express';
import cors from 'cors';

const app = express();
const api = createServer(app);
const ws = new Server(api, {
  cors: {
    origin: "*",
    methods: ["GET", "POST"]
  },
});

const HOST = 'localhost';
const PORT = 8080;

app.use(cors());

app.get('/', (req, res) => {
  res.sendFile(path.join(import.meta.dirname, 'index.html'))
});

app.post('/send', (req, res) => {
  if (!req.is('application/json')) {
    res.sendStatus(400);
    return
  }

  ws.emit('data', req.body)
  res.sendStatus(200)
})

ws.on('connection', (socket) => {
  console.log('a user connected');

  socket.on('data', (data) => ws.emit('data', data))
  socket.on('disconnect', () => {
    console.log('user disconnected');
  });
});

api.listen(PORT, () => {
  app.currentServer = {
    host: HOST ? HOST : "127.0.0.1",
    port: PORT,
  };
  console.log(`Server init on: http://${HOST ? HOST : "127.0.0.1"}:${PORT}`);
});
