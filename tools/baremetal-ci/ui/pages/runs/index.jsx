import { useEffect, useState } from "react";
import Head from "next/head";
import Link from "next/link";
import { Table } from "react-bootstrap";
import { fetchJson } from "@/lib/api";
import Moment from 'react-moment';

export default function Runs() {
    const [runs, setRuns] = useState([]);
    useEffect(() => {
        async function fetch() {
            setRuns(await fetchJson("/api/runs"));
        }

        fetch();
    }, []);

    return (
        <div>
            <Head>
              <title>BareMetal CI - Runs</title>
            </Head>

            <header>
                <h2 className="mt-4 mb-4">Runs</h2>
            </header>
            <main>
                <Table size="sm" striped bordered hover>
                    <thead>
                        <tr>
                            <th className="collapsing">Status</th>
                            <th className="collapsing">ID</th>
                            <th className="collapsing">Runner Name</th>
                            <th>Build</th>
                            <th>Created at</th>
                        </tr>
                    </thead>
                    <tbody>
                        {runs.map(build => (
                            <tr key={build.id}>
                                <td>
                                    <Link href="/runs/[id]" as={`/runs/${build.id}`}>
                                        <a>{build.id}</a>
                                    </Link>
                                </td>
                                <td>{build.description}</td>
                                <td>{build.arch}</td>
                                <td><Moment unix fromNow>{build.created_at}</Moment></td>
                                <td>{build.created_by}</td>
                            </tr>
                        ))}
                    </tbody>
                </Table>
            </main>
        </div>
    )
}
