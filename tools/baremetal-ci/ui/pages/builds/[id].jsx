import { useEffect, useState } from "react";
import Head from "next/head";
import { useRouter } from "next/router";
import { fetchJson } from "@/lib/api";
import Moment from 'react-moment';

export default function Builds() {
    const { query: { id } } = useRouter();
    const [build, setBuild] = useState({});
    useEffect(() => {
        async function fetch() {
            setBuild(await fetchJson(`/api/builds/${id}`));
        }

        fetch();
    }, []);

    return (
        <div>
            <Head>
                <title>BareMetal CI - Build {id}</title>
            </Head>

            <header>
                <h2 class="mt-4 mb-4 pb-2 border-bottom">Builds</h2>
            </header>
            <main>
                <dl class="row">
                    <dt class="col-sm-2">Status</dt>
                    <dd class="col-sm-10">{build.status}</dd>
                    <dt class="col-sm-2">Build ID</dt>
                    <dd class="col-sm-10">{build.id}</dd>
                    <dt class="col-sm-2">Git Commit</dt>
                    <dd class="col-sm-10">{build.commit}</dd>
                    <dt class="col-sm-2">Description</dt>
                    <dd class="col-sm-10">{build.description}</dd>
                    <dt class="col-sm-2">Architecture</dt>
                    <dd class="col-sm-10">{build.arch}</dd>
                    <dt class="col-sm-2">Created by</dt>
                    <dd class="col-sm-10">{build.created_by}</dd>
                    <dt class="col-sm-2">Created at</dt>
                    <dd class="col-sm-10">
                        <Moment unix>{build.created_at}</Moment>
                        &nbsp;(<Moment unix fromNow>{build.created_at}</Moment>)
                    </dd>
                </dl>
            </main>
        </div>
    )
}
