import Head from 'next/head';
import { Table } from 'react-bootstrap';
import { fetchJson } from './_app';

export async function getStaticProps(context) {
    const resp = await fetchJson("/api/builds");
    return {
      props: resp,
    }
}

export default function Builds() {
    return (
        <div>
            <Head>
              <title>BareMetal CI</title>
            </Head>

            <header>
                <h2>Builds</h2>
            </header>
            <main>
                <Table size="sm">
                    <thead>
                        <tr>
                            <th>Status</th>
                            <th>ID</th>
                            <th>Commit</th>
                            <th>Description</th>
                            <th>Arch</th>
                            <th>Created by</th>
                            <th>Created at</th>
                        </tr>
                    </thead>
                </Table>
            </main>
        </div>
    )
}
