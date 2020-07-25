import * as randomColor from "randomcolor";
import { PureComponent } from "react";
import { AutoSizer, Column, Table } from 'react-virtualized';
import { TableCell, createStyles, withStyles } from "@material-ui/core";
import clsx from 'clsx';

const styles = (theme) => createStyles({
    flexContainer: {
      display: 'flex',
      alignItems: 'center',
      boxSizing: 'border-box',
    },
    tableCell: {
        flex: 1,
    },
});

class MuiVirtualizedTable extends PureComponent {
    headerRenderer = ({ label }) => {
        const { headerHeight, classes } = this.props;
        return (
            <TableCell
                component="div"
                variant="head"
                className={clsx(classes.tableCell, classes.flexContainer)}
                style={{ height: headerHeight }}
            >{ label }</TableCell>
        )
    }

    cellRenderer = ({ cellData }) => {
        const { classes, rowHeight } = this.props;
        return (
            <TableCell
                component="div"
                variant="head"
                className={clsx(classes.tableCell, classes.flexContainer)}
                style={{ height: rowHeight }}
            >{ cellData }</TableCell>
        )
    }

    render() {
        const { classes, columns, headerHeight, rowHeight, rowCount, rowGetter } = this.props;
        return (
            <AutoSizer>
                {({ height, width }) => (
                    <Table
                        height={height}
                        width={width}
                        rowHeight={rowHeight}
                        rowCount={rowCount}
                        rowGetter={rowGetter}
                        headerHeight={headerHeight}
                        rowClassName={clsx(classes.tableRow, classes.flexContainer)}
                        gridStyle={{
                            direction: 'inherit',
                          }}
                    >
                        {columns.map(({ key, label, width }) => {
                            return (
                                <Column
                                    key={key}
                                    headerRenderer={this.headerRenderer}
                                    cellRenderer={this.cellRenderer}
                                    dataKey={key}
                                    width={width}
                                    label={label}
                                />
                            );
                        })}
                    </Table>
                )}
            </AutoSizer>
        );
    }
}

const VirtualizedTable = withStyles(styles)(MuiVirtualizedTable);

export default function LogStream({ items }) {
    return (
        <VirtualizedTable
            rowCount={items.length}
            rowGetter={({ index }) => items[index]}
            headerHeight={50}
            rowHeight={50}
            columns={[
                {
                    label: "Timestamp",
                    key: "timestamp",
                    width: 200,
                },
                {
                    label: "Task",
                    key: "task",
                    width: 100,
                },
                {
                    label: "Message",
                    key: "message",
                    width: 400,
                },
            ]}
        >
        </VirtualizedTable>
    )
}

/*
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
*/
