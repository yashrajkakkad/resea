import { ResponsiveLine } from "@nivo/line";

export default function TimeSeriesGraph({ data, yLegend, colorScheme }) {
    return (
        <ResponsiveLine
            data={data}
            margin={{ top: 20, right: 0, bottom: 50, left: 50 }}
            xScale={{ type: 'point' }}
            yScale={{ type: 'linear', min: 0, max: 'auto', stacked: true }}
            axisBottom={null}
            axisLeft={{
                orient: 'left',
                tickSize: 5,
                tickPadding: 5,
                tickRotation: 0,
                legend: yLegend,
                legendOffset: -40,
                legendPosition: 'middle'
            }}
            legends={[
                {
                    anchor: "bottom",
                    direction: "column",
                    symbolShape: "circle",
                    justify: true,
                    translateY: 30,
                    itemHeight: 20,
                    itemWidth: 80,
                    effects: []
                }
            ]}
            enableArea={true}
            animate={false}
            useMesh={true}
            colors={{ scheme: colorScheme ? colorScheme : "nivo" }}
        />
    )
}
