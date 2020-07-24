import Head from "next/head";
import MenuIcon from "@material-ui/icons/Menu";
import ChevronLeftIcon from "@material-ui/icons/ChevronLeft";
import DashboardIcon from "@material-ui/icons/Dashboard";
import { AppBar, Toolbar, IconButton, List, ListItem, Typography, CssBaseline, Drawer, Divider, ListItemIcon } from "@material-ui/core";
import { useState } from "react";

export default function App({ Component, pageProps }) {
    const [drawerOpened, setDrawerOpened] = useState(true);
    const handleDrawerOpen = () => { setDrawerOpened(true); };
    const handleDrawerClose = () => { setDrawerOpened(false); };
    return (
        <div>
            <CssBaseline />
            <Head>
                <title>Resea Analyzer</title>
                <link rel="stylesheet" href="https://fonts.googleapis.com/css?family=Roboto:300,400,500,700&display=swap" />
                <meta name="viewport" content="minimum-scale=1, initial-scale=1, width=device-width" />
                <meta name="robots" content="noindex, nofollow" />
            </Head>

            <AppBar position="static">
                <Toolbar>
                    <IconButton
                        edge="start" color="inherit" aria-label="menu"
                        onClick={handleDrawerOpen}
                    >
                        <MenuIcon />
                    </IconButton>
                    <Typography variant="h6">Resea Analyzer</Typography>
                </Toolbar>
            </AppBar>

            <Drawer
                variant="permanent"
                open={drawerOpened}
            >
                <div>
                  <IconButton onClick={handleDrawerClose}>
                    <ChevronLeftIcon />
                  </IconButton>
                </div>
                <Divider />
                <List component="nav" aria-label="navigation">
                    <ListItem button>
                        <ListItemIcon>
                            <DashboardIcon />
                        </ListItemIcon>
                    </ListItem>
                </List>
            </Drawer>

            <Component {...pageProps} />
        </div>
    )
}
