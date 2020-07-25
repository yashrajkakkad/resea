import io from 'socket.io-client';
import { useState, useEffect, useRef } from 'react';
import LogStream from "../components/log_stream";
import TimeSeriesGraph from "../components/time_series_graph";
import {
    Grid, Container, Paper, makeStyles, Typography, Box,
    Table, TableContainer, TableHead, TableBody, TableRow, TableCell
} from '@material-ui/core';

const useStyles = makeStyles((theme) => ({
    welcomePaper: {
      padding: theme.spacing(2),
      backgroundColor: theme.palette.grey[200],
      marginBottom: theme.spacing(1),
    },
    graphPaper: {
        padding: theme.spacing(1),
        display: 'flex',
        overflow: 'auto',
        flexDirection: 'column',
    },
    graph: {
        height: 200,
    },
    logPaper: {
        marginTop: theme.spacing(2),
        padding: theme.spacing(2),
    },
    logStream: {
        marginTop: theme.spacing(1),
        height: 200,
        overflow: "scroll",
    },
    tasksPaper: {
        marginTop: theme.spacing(2),
        padding: theme.spacing(2),
    },
    tasksTable: {
        marginTop: theme.spacing(2),
    }
}));

export default function Home() {
    const classes = useStyles();
    const [tasks, setTasks] = useState([]);
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
        socket.current.on("tasks", ({ tasks }) => {
            setTasks(tasks);
        });

        socket.current.on("log", e => {
            setLogEntries(prev => [...prev, e])
        });

        socket.current.on("sensor", e => {
            switch (e.key) {
                case "kernel.cpu_load":
                    setCpuLoadData(prev =>
                        [...prev.slice(-20), { x: e.timestamp, y: e.value }]
                    );
                break;
                case "kernel.mem_used":
                    setKernelMemUsedData(prev =>
                        [...prev.slice(-20), { x: e.timestamp, y: e.value }]
                    );
                break;
                case "bootstrap.mem_used":
                    setUserMemUsedData(prev =>
                        [...prev.slice(-20), { x: e.timestamp, y: e.value }]
                    );
                break;
            }
        });
    }, [socket]);

    return (
        <Container maxWidth="lg">
            <Paper className={classes.welcomePaper} elevation={0}>
                <Typography variant="body1">
                    Resea Analyzer analyzes the kernel log and visualizes
                    system status and events.
                </Typography>
            </Paper>
            <Grid container spacing={1}>
                <Grid item xs={12} md={4} lg={3}>
                    <Paper className={classes.graphPaper}>
                        <Typography component="h2" variant="h6">
                            CPU load
                        </Typography>
                        <div className={classes.graph}>
                            <TimeSeriesGraph
                                data={[ { id: "cpu_load", data: cpuLoadData } ]}
                                yLegend="# of tasks in runqueue"
                                colorScheme="nivo"
                            />
                        </div>
                    </Paper>
                </Grid>
                <Grid item xs={12} md={4} lg={3}>
                    <Paper className={classes.graphPaper}>
                        <Typography component="h2" variant="h6">
                            Memory usage
                        </Typography>
                            <div className={classes.graph}>
                            <TimeSeriesGraph
                                data={[ { id: "mem_used", data: kernelMemUsedData } ]}
                                yLegend="MiB"
                                colorScheme="category10"
                            />
                        </div>
                    </Paper>
                </Grid>
                <Grid item xs={12} md={4} lg={3}>
                    <Paper className={classes.graphPaper}>
                        <Typography component="h2" variant="h6">
                            IPC
                        </Typography>
                        <div className={classes.graph}>
                            <TimeSeriesGraph
                                data={[ { id: "mem_used", data: userMemUsedData } ]}
                                yLegend="MiB"
                                colorScheme="category10"
                            />
                        </div>
                    </Paper>
                </Grid>
            </Grid>

            <Paper className={classes.logPaper}>
                <Typography component="h2" variant="h6">
                    Log
                </Typography>
                <Box className={classes.logStream}>
                    <LogStream items={logItems}></LogStream>
                </Box>
            </Paper>

            <Paper className={classes.tasksPaper}>
                <Typography component="h2" variant="h6">
                    Tasks
                </Typography>
                <TableContainer className={classes.tasksTable}>
                    <Table size="small">
                        <TableHead>
                            <TableRow>
                                <TableCell>Task ID</TableCell>
                                <TableCell>Name</TableCell>
                                <TableCell>State</TableCell>
                            </TableRow>
                        </TableHead>
                        <TableBody>
                            {tasks.map(task => (
                                <TableRow key={task.id}>
                                    <TableCell>{task.id}</TableCell>
                                    <TableCell>{task.name}</TableCell>
                                    <TableCell>
                                        {task.state}
                                        {task.state == "sending"
                                         && ` (to #${task.src_or_dst})`}
                                        {task.state == "receiving"
                                         && task.src
                                         && ` (from #${task.src_or_dst})`}
                                        {task.state == "receiving"
                                         && !task.src
                                         && ` (from any)`}
                                    </TableCell>
                                </TableRow>
                            ))}
                        </TableBody>
                    </Table>
                </TableContainer>
            </Paper>
        </Container>
    )
}
