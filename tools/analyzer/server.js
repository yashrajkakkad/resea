#!/usr/bin/env node
const io = require("socket.io")()

const startedAt = Math.floor(new Date());

setInterval(() => {
    io.emit("sensor", ({
        timestamp: (Math.floor(new Date()) - startedAt) / 1000,
        key: "kernel.cpu_load",
        value: Math.floor(Math.random() * 10)
    }))

    io.emit("sensor", ({
        timestamp: (Math.floor(new Date()) - startedAt) / 1000,
        key: "kernel.mem_used",
        value: 300 + Math.floor(Math.random() * 100)
    }))
}, 1000)

setInterval(() => {
    io.emit("log", ({
        timestamp: (Math.floor(new Date()) - startedAt) / 1000,
        task: "kernel",
        level: "INFO",
        message: `random string ${Math.floor(Math.random() * 100)}`,
    }))
}, 3000)

io.on('connection', client => { console.log(client) });

const PORT = 9091
io.listen(PORT)
console.log("* available at ws://localhost:9091")
