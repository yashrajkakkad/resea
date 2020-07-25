import Head from "next/head";
import MenuIcon from "@material-ui/icons/Menu";
import FeedbackIcon from "@material-ui/icons/Feedback";
import SettingsIcon from "@material-ui/icons/Settings";
import {
    AppBar, Toolbar, Typography, CssBaseline,
    useMediaQuery, createMuiTheme, ThemeProvider,
    Tabs, Tab, Paper, Box, Button, Grid, FormControl, InputLabel, Input, InputAdornment, TextField
} from "@material-ui/core";
import { makeStyles, useTheme } from '@material-ui/core/styles';
import { useMemo, useState } from "react";

const useStyles = makeStyles((theme) => ({
    title: {
        flexGrow: 1,
    },
    toolbar: {
        minHeight: 48,
    },
    content: {
        marginTop: theme.spacing(1),
    },
    streamUrl: {
        marginRight: theme.spacing(2),
        marginLeft: theme.spacing(3),
        flexGrow: 1,
        maxWidth: "500px",
    }
}));

export default function App({ Component, pageProps }) {
    const [streamUrl, setStreamUrl] = useState("http://localhost:9091");
    const prefersDarkMode = useMediaQuery('(prefers-color-scheme: dark)');
    const theme = useMemo(() => {
        return createMuiTheme({
          palette: { type: prefersDarkMode ? 'dark' : 'light' },
        })
    }, [prefersDarkMode]);

    const updateStreamUrl = (ev) => {
        setStreamUrl(ev.target.value);
    }

    const classes = useStyles(theme);
    return (
        <ThemeProvider theme={theme}>
            <CssBaseline />
            <Head>
                <title>Resea Analyzer</title>
                <link rel="stylesheet" href="https://fonts.googleapis.com/css?family=Roboto:300,400,500,700&display=swap" />
                <meta name="viewport" content="minimum-scale=1, initial-scale=1, width=device-width" />
                <meta name="robots" content="noindex, nofollow" />
            </Head>

            <AppBar position="fixed">
                <Toolbar variant="dense">
                    <Typography variant="h6" className={classes.title}>Resea Analyzer</Typography>
                    <Button
                        color="inherit" startIcon={<FeedbackIcon />}
                        onClick={() => { window.open("https://gitter.im/resea/community", "_blank").focus() }}
                    >
                        Feedback
                    </Button>
                </Toolbar>
            </AppBar>

            <main>
                <div className={classes.toolbar} />
                <Paper elevation={0} variant="outlined">
                    <Grid container justify="space-between"  alignItems="center">
                        <Grid item>
                            <Tabs
                                value={0}
                                onChange={() => {}}
                                indicatorColor="primary"
                                textColor="primary"
                            >
                                <Tab label="dashboard" />
                            </Tabs>
                        </Grid>
                        <Grid item className={classes.streamUrl}>
                            <TextField
                                label="Log stream URL"
                                variant="filled"
                                fullWidth={true}
                                value={streamUrl}
                                onChange={updateStreamUrl}
                            />
                        </Grid>
                    </Grid>
                </Paper>
                <Box className={classes.content}>
                    <Component streamUrl={streamUrl} {...pageProps} />
                </Box>
            </main>
        </ThemeProvider>
    )
}
