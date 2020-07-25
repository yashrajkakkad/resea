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
        </div>
    )
}
