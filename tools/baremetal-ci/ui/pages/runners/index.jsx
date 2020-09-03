import { useEffect, useState } from "react";
import Head from "next/head";
import { Table } from "react-bootstrap";
import { fetchJson } from "@/lib/api";
import Moment from 'react-moment';

export default function Runners() {
    const [runners, setRunners] = useState([]);
    useEffect(() => {
        async function fetch() {
            setRunners(await fetchJson("/api/runners"));
        }

        fetch();
    }, []);

    return (
        <div>
            <Head>
              <title>BareMetal CI - Runners</title>
            </Head>

            <header>
                <h2 className="mt-4 mb-4">Runners</h2>
            </header>
            <main>
                <Table size="sm" striped bordered hover>
                    <thead>
                        <tr>
                            <th>ID</th>
                            <th>Name</th>
                            <th>Machine</th>
                            <th>Updated at</th>
                        </tr>
                    </thead>
                    <tbody>
                        {runners.map(runner => (
                            <tr key={runner.id}>
                                <td>{runner.id}</td>
                                <td>{runner.name}</td>
                                <td>{runner.machine}</td>
                                <td><Moment unix fromNow>{runner.updated_at}</Moment></td>
                            </tr>
                        ))}
                    </tbody>
                </Table>
            </main>
        </div>
    )
}
