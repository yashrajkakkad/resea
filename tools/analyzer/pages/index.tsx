import io from 'socket.io-client';
import { useState, useEffect, useRef } from 'react';
import LogStream from "../components/log_stream";
import TimeSeriesGraph from "../components/time_series_graph";
import {
    Grid, Container, Paper, makeStyles, Typography, Box,
    Table, TableContainer, TableHead, TableBody, TableRow, TableCell,
    Card, CardContent, CardActions, Button, FormControlLabel, Switch, Divider,
} from '@material-ui/core';

const useStyles = makeStyles((theme) => ({
    papersContainer: {
    },
    graphPaper: {
        padding: theme.spacing(1),
        margin: theme.spacing(1),
        marginBottom: theme.spacing(2),
        display: 'flex',
        overflow: 'auto',
        flexDirection: 'column',
    },
    graph: {
        height: 200,
    },
    logGrid: {
        flexGrow: 1,
    },
    logCard: {
        margin: theme.spacing(1),
    },
    logStream: {
        marginTop: theme.spacing(1),
        minHeight: "500px", // FIXME: Remove this hard-coded size.
        overflow: "scroll",
    },
    tasksPaper: {
        padding: theme.spacing(2),
        marginBottom: theme.spacing(2),
    },
    tasksTable: {
        marginTop: theme.spacing(2),
    }
}));

export default function Home({ streamUrl }) {
    const classes = useStyles();
    const [tasks, setTasks] = useState([]);
    const [cpuLoadData, setCpuLoadData] = useState([]);
    const [kernelMemUsedData, setKernelMemUsedData] = useState([]);
    const [userMemUsedData, setUserMemUsedData] = useState([]);
    const [logItems, setLogEntries] = useState([]);
    const socket = useRef(null);
    const [autoScroll, setAutoScroll] = useState(true);

    const toggleAutoScroll = (ev) => {
        setAutoScroll(ev.target.checked);
    }

    useEffect(() => {
        if (socket.current) {
            socket.current.close();
        }

        socket.current = io(streamUrl);
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

        return () => {
            socket.current.close();
        }
    }, [streamUrl]);

    return (
        <Container maxWidth="xl">
            <Grid container spacing={1} className={classes.papersContainer}>
                <Grid container item xs={12} lg={6} alignContent="flex-start">
                    <Grid item xs={12} md={4} lg={4}>
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
                    <Grid item xs={12} md={4} lg={4}>
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
                    <Grid item xs={12} md={4} lg={4}>
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
                    <Grid item xs={12} md={12} lg={12}>
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
                    </Grid>
                </Grid>

                <Grid item xs={12} lg={6} className={classes.logGrid}>
                    <Card className={classes.logCard}>
                        <CardContent>
                            <Typography component="h2" variant="h6">
                                Log
                            </Typography>
                            <Box className={classes.logStream}>
                                <LogStream items={logItems} autoScroll={autoScroll}></LogStream>
                            </Box>
                        </CardContent>
                        <Divider />
                        <CardActions>
                            <Grid container justify="space-between"  alignItems="center">
                                <Grid item>
                                    <FormControlLabel
                                        control={<Switch checked={autoScroll} onChange={toggleAutoScroll} />}
                                        label={autoScroll ? "Auto Scrolling..." : "Auto Scroll"}
                                    />
                                </Grid>
                                <Grid item>
                                    <Typography>
                                        {logItems.length} lines in total
                                    </Typography>
                                </Grid>
                            </Grid>
                        </CardActions>
                    </Card>
                </Grid>
            </Grid>
        </Container>
    )
}
