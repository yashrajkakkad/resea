import Head from "next/head";
import "../theme.scss";
import { ThemeProvider } from "@nivo/core";

const theme = {
    axis: {
        textColor: '#eee',
        fontSize: '14px',
        tickColor: '#eee',
      },
      grid: {
        stroke: '#888',
        strokeWidth: 1
      }
}

export default function App({ Component, pageProps }) {
    return (
        <div>
            <Head>
                <title>Resea Analyzer</title>
            </Head>
            <ThemeProvider theme={theme}>
                <Component {...pageProps} />
            </ThemeProvider>
        </div>
    )
}
