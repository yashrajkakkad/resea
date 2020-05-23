import io from 'socket.io-client';
import { useState, useEffect, useRef } from 'react';
import LogStream from "../components/log_stream";

export default function Home() {
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
            setLogEntries(logItems => [...logItems, e])
        });
    }, [socket]);

    return (<div>
        <h1>Resea Analyzer</h1>
        <div style={{ height: "10vh", width: "100%" }}>
            <LogStream items={logItems}></LogStream>
        </div>
    </div>)
}
