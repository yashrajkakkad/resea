import { useEffect, useState } from "react";
import Head from "next/head";
import { useRouter } from "next/router";
import { fetchJson } from "@/lib/api";
import Moment from "react-moment";

export default function Builds() {
    const router = useRouter();
    const [build, setBuild] = useState({});
    useEffect(() => {
        async function fetch() {
            if (!router.query.id) return;
            setBuild(await fetchJson(`/api/builds/${router.query.id}`));
        }

        fetch();
    }, [router]);

    return (
        <div>
            <Head>
                <title>BareMetal CI - Build {build.id}</title>
            </Head>

            <header>
                <h2 className="mt-4 mb-4 pb-2 border-bottom">Build - { build.description || build.id}</h2>
            </header>
            <main>
                <section>
                    <dl className="row">
                        <dt className="col-sm-2">Build ID</dt>
                        <dd className="col-sm-10">{build.id}</dd>
                        <dt className="col-sm-2">Git Commit</dt>
                        <dd className="col-sm-10">{build.commit}</dd>
                        <dt className="col-sm-2">Description</dt>
                        <dd className="col-sm-10">{build.description}</dd>
                        <dt className="col-sm-2">Architecture</dt>
                        <dd className="col-sm-10">{build.arch}</dd>
                        <dt className="col-sm-2">Created by</dt>
                        <dd className="col-sm-10">{build.created_by}</dd>
                        <dt className="col-sm-2">Created at</dt>
                        <dd className="col-sm-10">
                            <Moment unix>{build.created_at}</Moment>
                            &nbsp;(<Moment unix fromNow>{build.created_at}</Moment>)
                        </dd>
                    </dl>
                </section>
            </main>
        </div>
    )
}
