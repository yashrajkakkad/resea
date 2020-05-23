export default function Block({ title, style, children }) {
    return (
        <section style={style}>
            {title && <h1>{title}</h1>}
            {children}
            <style jsx>{`
                section {
                    background: #fafafa;
                    padding: 5px 10px;
                    border-radius: 5px;
                    box-shadow: 3px 3px 0 0 #cacaca;
                }

                section:not(:first-child) {
                    margin-top: 30px;
                }
            `}</style>
        </section>
    )
}
