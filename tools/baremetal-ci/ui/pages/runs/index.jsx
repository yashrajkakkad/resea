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
                            <th>Status</th>
                            <th>ID</th>
                            <th>Runner Name</th>
                            <th>Build</th>
                            <th>Duration</th>
                            <th>Created at</th>
                        </tr>
                    </thead>
                    <tbody>
                        {runs.map(run => (
                            <tr key={run.id}>
                                <td>{run.status}</td>
                                <td>
                                    <Link href="/runs/[id]" as={`/runs/${run.id}`}>
                                        <a>{run.id}</a>
                                    </Link>
                                </td>
                                <td>{run.runner_name}</td>
                                <td>{run.build_id}</td>
                                <td>{run.duration && `${run.duration} seconds`}</td>
                                <td><Moment unix fromNow>{run.created_at}</Moment></td>
                            </tr>
                        ))}
                    </tbody>
                </Table>
            </main>
        </div>
    )
}
