import * as randomColor from "randomcolor";

export default function LogStream({ items }) {
    return (
        <div className="log-stream">
            {items.map(e => (
                <p key={e.timestamp} className="item">
                    <span className="timestamp">{e.timestamp}</span>
                    <span className="task" style={{
                        background: randomColor({
                            luminosity: 'dark',
                            seed: e.task
                        })
                    }}>{e.task}</span>
                    <span className="message">{e.message}</span>
                </p>
            ))}
            <style jsx>{`
                .log-stream {
                    height: 100%;
                    background: #f2f2f2;
                    padding: 5px 10px;
                    box-sizing: border-box;
                    width: 100%;
                    overflow: scroll;
                    font-family: var(--font-code);
                    font-size: 0.9rem;
                }

                .item {
                    margin: 5px 0;
                }

                .timestamp {
                    display: inline-block;
                    width: 80px;
                }

                .task {
                    display: inline-block;
                    width: 80px;
                    color: white;
                    font-weight: bold;
                    font-size: 0.78rem;
                    border-radius: 3px;
                    padding: 1px 5px;
                    text-align: center;
                }

                .level {
                    display: inline-block;
                }

                .message {
                    display: inline-block;
                    margin-left: 1rem;
                }
            `}</style>
        </div>
    )
}
