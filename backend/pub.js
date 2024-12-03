import mqtt from 'mqtt'

const client = mqtt.connect('mqtt://localhost')

client.on('connect', function () {
  setInterval(function() {
    client.publish('Topic test', 'Hallo SIJA')
    console.log('Message Sent')
  }, 5000)
})
