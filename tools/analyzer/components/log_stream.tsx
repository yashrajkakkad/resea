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
                            <td>{e.task}</td>
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
                    height: 100%;
                    width: 100%;
                    overflow: scroll;
                }

                .items {
                    border-collapse: collapse;
                    height: 100%;
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

                .anchor {
                    overflow-anchor: auto;
                    height: 1px;
                }
            `}</style>
        </div>
    )
}
