
#let base-styles(body) = {

  set page(
    margin: (x: 4em, y: 4em),
  )

  set heading(numbering: "1.1")
  show heading.where(level: 1): set align(center)
  show heading.where(level: 1): set block(below: 32pt)
  show heading.where(level: 2): set block(below: 16pt)
  show heading.where(level: 2): set block(below: 16pt)

  show raw.where(block: false): it => (
    " "
      + box(
        fill: rgb("#0001"),
        stroke: 0.5pt + rgb("#0001"),
        outset: (y: 3pt, x: 4pt),
        radius: 2pt,
        it,
      )
      + " "
  )

  body

}
