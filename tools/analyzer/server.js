#!/usr/bin/env node
const io = require("socket.io")()

setInterval(() => {
    io.emit("message", "hello!")
}, 3000)

io.on('connection', client => { console.log(client) });

const PORT = 9091
io.listen(PORT)
console.log("* available at ws://localhost:9091")
