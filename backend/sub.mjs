import mqtt from 'mqtt'

const client = mqtt.connect('mqtt://localhost')

client.on('connect', function () {
  client.subscribe('Topic test')
})

client.on('message', function (topic, message) {
  context = message.toString()
  console.log(context)
})
