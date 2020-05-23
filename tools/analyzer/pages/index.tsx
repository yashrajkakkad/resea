import io from 'socket.io-client';
import { useState, useEffect, useRef } from 'react';
import LogStream from "../components/log_stream";
import TimeSeriesGraph from "../components/time_series_graph";

export default function Home() {
    const [cpuLoadData, setCpuLoadData] = useState([]);
    const [memUsedData, setMemUsedData] = useState([]);
    const [logItems, setLogEntries] = useState([]);
    const socket = useRef(null);

    useEffect(() => {
        socket.current = io("http://localhost:9091")
        return () => {
            socket.current.close();
        }
    }, []);

    useEffect(() => {
        socket.current.on("log", e => {
            setLogEntries(prev => [...prev, e])
        });

        socket.current.on("sensor", e => {
            console.log(e)
            switch (e.key) {
                case "kernel.cpu_load":
                    setCpuLoadData(prev =>
                        [...prev.slice(-50), { x: e.timestamp, y: e.value }]
                    );
                break;
                case "kernel.mem_used":
                    setMemUsedData(prev =>
                        [...prev.slice(-50), { x: e.timestamp, y: e.value }]
                    );
                break;
            }
        });
    }, [socket]);

    return (<div>
        <h1>Resea Analyzer</h1>
        <div className="stats" style={{ height: "40vh", display: "flex" }}>
            <TimeSeriesGraph
             data={[ { id: "cpu_load", data: cpuLoadData } ]}
             yLegend="# of tasks in runqueue"
             colorScheme="nivo"
            />
            <TimeSeriesGraph
             data={[ { id: "mem_used", data: memUsedData } ]}
             yLegend="MiB"
             colorScheme="category10"
            />
        </div>
        <div style={{ height: "10vh", width: "100%" }}>
            <LogStream items={logItems}></LogStream>
        </div>
    </div>)
}
