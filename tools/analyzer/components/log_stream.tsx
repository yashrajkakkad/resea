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
                            <td style={{ width: "15%" }}>{e.timestamp}</td>
                            <td style={{ width: "10%" }}>
                                <span style={{
                                    color: "white",
                                    fontWeight: "bold",
                                    fontSize: "0.78rem",
                                    borderRadius: "3px",
                                    padding: "1px 5px",
                                    textAlign: "center",
                                    background: randomColor({
                                        luminosity: 'dark',
                                        seed: e.task
                                    })
                                }}>{e.task}</span>
                            </td>
                            <td style={{ width: "10%" }}>{e.level}</td>
                            <td>{e.message}</td>
                        </tr>
                    ))}
                    <tr className="anchor">
                        <td colSpan="4">
                            <span className="anchor-message">
                                bottom of log
                            </span>
                        </td>
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
                    padding: 4px 0px;
                }

                .anchor {
                    overflow-anchor: auto;
                    height: 1px;
                    text-align: center;
                }

                .anchor-message {
                    border: 1px solid #7a7a7a;
                    background: #efefef;
                    padding: 2px 20px;
                    border-radius: 5px;
                    font-weight: bold;
                }
            `}</style>
        </div>
    )
}
