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
        value: 10 + Math.floor(Math.random() * 30)
    }))

    io.emit("sensor", ({
        timestamp: (Math.floor(new Date()) - startedAt) / 1000,
        key: "bootstrap.mem_used",
        value: 100 + Math.floor(Math.random() * 100)
    }))
}, 1000)

setInterval(() => {
    io.emit("log", ({
        timestamp: (Math.floor(new Date()) - startedAt) / 1000,
        task: "kernel",
        level: "INFO",
        message: `random string ${Math.floor(Math.random() * 100)}`,
    }))

    io.emit("tasks", ({
        tasks: [
            {
                id: 1,
                name: "bootstrap",
                state: "receiving",
                src_or_dst: 0,
                cpu: 20,
                memory: 30,
            },
            {
                id: 2,
                name: "ramdisk",
                state: "sending",
                src_or_dst: 1,
                cpu: 10,
                memory: 30,
            },
            {
                id: 3,
                name: "fatfs",
                state: "runnable",
                src_or_dst: 0,
                cpu: 20,
                memory: 50,
            },
        ]
    }))
}, 3000)

io.on('connection', client => { console.log(client) });

const PORT = 9091
io.listen(PORT)
console.log("* available at ws://localhost:9091")
