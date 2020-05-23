import io from 'socket.io-client';
import { useState, useEffect, useRef } from 'react';
import LogStream from "../components/log_stream";
import TimeSeriesGraph from "../components/time_series_graph";
import Block from "../components/block";

export default function Home() {
    const [cpuLoadData, setCpuLoadData] = useState([]);
    const [kernelMemUsedData, setKernelMemUsedData] = useState([]);
    const [userMemUsedData, setUserMemUsedData] = useState([]);
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
                    setKernelMemUsedData(prev =>
                        [...prev.slice(-50), { x: e.timestamp, y: e.value }]
                    );
                break;
                case "bootstrap.mem_used":
                    setUserMemUsedData(prev =>
                        [...prev.slice(-50), { x: e.timestamp, y: e.value }]
                    );
                break;
            }
        });
    }, [socket]);

    return (
        <div style={{ margin: "0px 20px" }}>
            <h1>Resea Analyzer</h1>
            <div style={{ display: "flex" }}>
                <div style={{ "flex-grow": "1" }}>
                    <Block className="stats" style={{ height: "200px", display: "flex" }}>
                        <TimeSeriesGraph
                         data={[ { id: "cpu_load", data: cpuLoadData } ]}
                         yLegend="# of tasks in runqueue"
                         colorScheme="nivo"
                         />
                        <TimeSeriesGraph
                         data={[ { id: "mem_used", data: kernelMemUsedData } ]}
                         yLegend="MiB"
                         colorScheme="category10"
                         />
                        <TimeSeriesGraph
                         data={[ { id: "mem_used", data: userMemUsedData } ]}
                         yLegend="MiB"
                         colorScheme="category10"
                         />
                    </Block>
                    <Block title="Log" style={{ height: "300px" }}>
                        <LogStream items={logItems}></LogStream>
                    </Block>
                </div>
                <div style={{ width: "350px", "margin-left": "20px" }}>
                    <Block title="Tasks">
                        foo
                    </Block>
                </div>
            </div>
        </div>
    )
}
