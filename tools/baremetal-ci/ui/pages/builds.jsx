import { useEffect, useState } from "react";
import Head from "next/head";
import { Table } from "react-bootstrap";
import { fetchJson } from "./_app";

export default function Builds() {
    const [builds, setBuilds] = useState([]);
    useEffect(() => {
        async function fetch() {
            setBuilds(await fetchJson("/api/builds"));
        }

        fetch();
    });

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
                    <tbody>
                        {builds.map(build => (
                            <tr key={build.id}>
                                <td>{build.status}</td>
                                <td>{build.id}</td>
                                <td>{build.commit}</td>
                                <td>{build.description}</td>
                                <td>{build.arch}</td>
                                <td>{build.created_by}</td>
                                <td>{build.created_at}</td>
                            </tr>
                        ))}
                    </tbody>
                </Table>
            </main>
        </div>
    )
}
