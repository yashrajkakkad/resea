import Head from "next/head";
import MenuIcon from "@material-ui/icons/Menu";
import DashboardIcon from "@material-ui/icons/Dashboard";
import {
    AppBar, Toolbar, IconButton, List, ListItem,
    Typography, CssBaseline, Drawer, Divider,
    ListItemIcon, ListItemText, useMediaQuery, createMuiTheme, ThemeProvider
} from "@material-ui/core";
import { makeStyles, useTheme } from '@material-ui/core/styles';
import { useMemo } from "react";

const drawerWidth = 180;
const useStyles = makeStyles((theme) => ({
    root: {
      display: 'flex',
    },
    drawer: {
      [theme.breakpoints.up('sm')]: {
        width: drawerWidth,
        flexShrink: 0,
      },
    },
    appBar: {
      [theme.breakpoints.up('sm')]: {
        width: `calc(100% - ${drawerWidth}px)`,
        marginLeft: drawerWidth,
      },
    },
    toolbar: theme.mixins.toolbar,
    drawerPaper: {
      width: drawerWidth,
    },
    content: {
        flexGrow: 1,
        padding: theme.spacing(1),
    },
}));

export default function App({ Component, pageProps }) {
    const prefersDarkMode = useMediaQuery('(prefers-color-scheme: dark)');
    const theme = useMemo(() => {
        return createMuiTheme({
          palette: { type: prefersDarkMode ? 'dark' : 'light' },
        })
    }, [prefersDarkMode]);

    const classes = useStyles(theme);
    return (
        <ThemeProvider theme={theme}>
            <CssBaseline />
            <div className={classes.root}>
                <Head>
                    <title>Resea Analyzer</title>
                    <link rel="stylesheet" href="https://fonts.googleapis.com/css?family=Roboto:300,400,500,700&display=swap" />
                    <meta name="viewport" content="minimum-scale=1, initial-scale=1, width=device-width" />
                    <meta name="robots" content="noindex, nofollow" />
                </Head>

                <AppBar position="fixed" className={classes.appBar}>
                    <Toolbar variant="dense">
                        <IconButton
                            edge="start" color="inherit" aria-label="menu"
                        >
                            <MenuIcon />
                        </IconButton>
                        <Typography variant="h6">Resea Analyzer</Typography>
                    </Toolbar>
                </AppBar>

                <nav className={classes.drawer}>
                    <Drawer
                        variant="permanent"
                        open={true}
                        classes={{ paper: classes.drawerPaper }}
                    >
                        <List component="nav" aria-label="navigation">
                            <ListItem button>
                                <ListItemIcon>
                                    <DashboardIcon />
                                </ListItemIcon>
                                <ListItemText primary="Dashboard" />
                            </ListItem>
                        </List>
                    </Drawer>
                </nav>

                <main className={classes.content}>
                    <div className={classes.toolbar} />
                    <Component {...pageProps} />
                </main>
            </div>
        </ThemeProvider>
    )
}
