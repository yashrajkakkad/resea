import "bootstrap/dist/css/bootstrap.min.css";
import Link from "next/link";
import { Navbar, Nav, Container } from "react-bootstrap";

export default function App({ Component, pageProps }) {
    return (
        <div>
            <Navbar bg="light">
            <Navbar.Brand href="#home">BareMetal CI</Navbar.Brand>
            <Nav className="mr-auto">
                <Link passHref href="/builds"><Nav.Link>Builds</Nav.Link></Link>
                <Link passHref href="/tests"><Nav.Link>Tests</Nav.Link></Link>
                <Link passHref href="/runners"><Nav.Link>Runners</Nav.Link></Link>
            </Nav>
            <Nav>
                <Nav.Link href="https://github.com/nuta/resea">GitHub</Nav.Link>
            </Nav>
            </Navbar>
            <Container>
                <Component {...pageProps} />
            </Container>
        </div>
    )
}
