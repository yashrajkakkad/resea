import Head from "next/head"
import "../theme.scss"

export default function App({ Component, pageProps }) {
    return (
        <div>
            <Head>
                <title>Resea Analyzer</title>
            </Head>
            <Component {...pageProps} />
        </div>
    )
}
