import * as randomColor from "randomcolor";

export default function LogStream({ items }) {
    return (
        <div className="log-stream">
            <table className="items">
                <thead>
                    <tr>
                        <th>Timestamp</th>
                        <th>Task</th>
                        <th>Level</th>
                        <th>Message</th>
                    </tr>
                </thead>
                <tbody>
                    {items.map(e => (
                        <tr key={e.timestamp} className="item">
                            <td>{e.timestamp}</td>
                            <td>
                                <span style={{
                                    color: "white",
                                    fontWeight: "bold",
                                    fontSize: "0.8rem",
                                    borderRadius: "3px",
                                    padding: "2px 5px",
                                    textAlign: "center",
                                    background: randomColor({
                                        luminosity: 'dark',
                                        seed: e.task
                                    })
                                }}>{e.task}</span>
                            </td>
                            <td>{e.level}</td>
                            <td>{e.message}</td>
                        </tr>
                    ))}
                    <tr className="anchor">
                        <td>bottom of log</td>
                    </tr>
                </tbody>
            </table>
            <style jsx>{`
                .log-stream {
                    max-height: 200px;
                    width: 100%;
                    overflow: scroll;
                    font-family: var(--font-code);
                    font-size: 0.9rem;
                }

                .items {
                    border-collapse: collapse;
                    width: 100%;
                }

                .item {
                    overflow-anchor: none;
                }

                thead th {
                    position: sticky;
                    top: 0;
                    z-index: 1;
                    background: #efefef;
                }

                tbody td {
                    padding: 4px 10px;
                }

                .anchor {
                    overflow-anchor: auto;
                    height: 1px;
                }
            `}</style>
        </div>
    )
}
