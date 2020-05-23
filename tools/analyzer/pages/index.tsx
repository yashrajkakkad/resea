import io from 'socket.io-client';
import { useState, useEffect, useRef } from 'react';
import LogStream from "../components/log_stream";
import { ResponsiveLine } from "@nivo/line";

export default function Home() {
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
            setMemUsedData(prev =>
                [...prev.slice(-50), { x: e.timestamp, y: e.value }]
            );
        });
    }, [socket]);

    return (<div>
        <h1>Resea Analyzer</h1>
        <div className="stats" style={{ height: "40vh", display: "flex" }}>
            <ResponsiveLine
                data={[ { id: "mem_used", data: memUsedData } ]}
                margin={{ top: 50, right: 110, bottom: 50, left: 60 }}
                xScale={{ type: 'time', format: '%s' }}
                xFormat="time:%s"
                yScale={{ type: 'linear', min: 0, max: 'auto', stacked: true }}
                axisBottom={{
                    orient: 'bottom',
                    format: '%H:%M:%S',
                    tickPadding: 5,
                    tickRotation: 45,
                }}
                axisLeft={{
                    orient: 'left',
                    tickSize: 5,
                    tickPadding: 5,
                    tickRotation: 0,
                    legend: 'MiB',
                    legendOffset: -40,
                    legendPosition: 'middle'
                }}
                legends={[
                    {
                        anchor: "bottom-right",
                        direction: "column",
                        symbolShape: 'circle',
                        justify: true,
                        translateX: 100,
                        itemHeight: 20,
                        itemWidth: 80,
                        effects: []
                    }
                ]}
                enableArea={true}
                animate={false}
                useMesh={true}
            />
        </div>
        <div style={{ height: "10vh", width: "100%" }}>
            <LogStream items={logItems}></LogStream>
        </div>
    </div>)
}
