import { useEffect, useState } from "react";
import Head from "next/head";
import Link from "next/link";
import { Table } from "react-bootstrap";
import { fetchJson } from "@/lib/api";
import Moment from 'react-moment';

export default function Builds() {
    const [builds, setBuilds] = useState([]);
    useEffect(() => {
        async function fetch() {
            setBuilds(await fetchJson("/api/builds"));
        }

        fetch();
    }, []);

    return (
        <div>
            <Head>
              <title>BareMetal CI</title>
            </Head>

            <header>
                <h2 className="mt-4 mb-4">Builds</h2>
            </header>
            <main>
                <Table size="sm" striped bordered hover>
                    <thead>
                        <tr>
                            <th>ID</th>
                            <th>Description</th>
                            <th>Arch</th>
                            <th>Created at</th>
                            <th>Created by</th>
                        </tr>
                    </thead>
                    <tbody>
                        {builds.map(build => (
                            <tr key={build.id}>
                                <td>
                                    <Link href="/builds/[id]" as={`/builds/${build.id}`}>
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
