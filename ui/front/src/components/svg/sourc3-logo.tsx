type Sourc3LogoProps = {
  fill: string;
  className?: string;
};

function Sourc3Logo({ fill, className = '' }:Sourc3LogoProps) {
  return (
    <svg
      className={className}
      width="268"
      height="87"
      viewBox="0 0 268 87"
      fill={fill}
      xmlns="http://www.w3.org/2000/svg"
    >
      <path
        fillRule="evenodd"
        clipRule="evenodd"
        d="M27.7727 38.7343L27.7726 38.7343C24.2703 37.7466 19.8257 36.4932 19.8257 33.5037C19.8257 31.0882 21.9334 29.5454 25.9448 29.5454C31.5881 29.5454 36.1434 32.5642 37.2991 34.711L44.2344 30.2832C42.5345 25.5871 35.5316 20.8906 27.0326 20.8906C18.0579 20.8906 9.76324 24.6476 9.76324 33.9733C9.76324 42.4901 18.7838 45.1088 23.1737 46.3831L23.1739 46.3832L23.1749 46.3835L23.1753 46.3836L23.1755 46.3836C23.6952 46.5345 24.1499 46.6665 24.517 46.7873C25.1179 46.9889 25.8293 47.1926 26.5932 47.4113L26.5935 47.4114L26.5936 47.4114C30.1519 48.4303 34.8516 49.776 34.8516 52.7584C34.8516 55.5093 33.3559 57.3876 27.9846 57.3876C21.6614 57.3876 16.3582 53.027 15.0664 50.5446L7.99542 55.5094C10.1712 60.7422 16.8342 66.1092 27.0327 66.1092C37.6392 66.1092 44.6424 61.4132 44.6424 52.3561C44.6424 44.0402 35.098 41.0187 30.8846 39.6848L30.8843 39.6847C30.4747 39.555 30.1156 39.4413 29.8204 39.3402C29.2143 39.1409 28.5146 38.9436 27.7727 38.7343ZM126.302 48.9351C126.302 54.9734 122.427 57.3885 117.94 57.3885C113.452 57.3885 109.577 54.9734 109.577 48.9351V21.6965H100.806V49.8746C100.806 58.529 106.925 65.8419 117.94 65.8419C128.954 65.8419 135.073 58.5289 135.073 49.8746V21.6965H126.302V48.9351ZM163.767 21.6965C170.362 21.6965 177.229 26.6612 177.229 36.0538C177.229 43.2995 173.081 47.9287 168.118 49.6733L178.113 65.3051H167.982L158.463 50.411H151.324V65.3051H142.553V21.6965H163.767ZM151.324 41.958H162.339C165.602 41.958 168.39 39.6766 168.39 36.0538C168.39 32.3641 165.602 30.0831 162.339 30.0831H151.324V41.958ZM204.631 29.8143C209.458 29.8143 213.334 32.2298 215.441 35.1146L222.512 29.9485C218.705 24.5143 212.45 21.1601 204.631 21.1601C191.509 21.1601 182.33 30.5526 182.33 43.5007C182.33 56.4493 191.509 65.8418 204.631 65.8418C212.45 65.8418 218.705 62.4872 222.512 57.0529L215.441 51.6859C213.334 54.4365 209.458 57.187 204.631 57.187C196.54 57.187 191.101 51.4173 191.101 43.5007C191.101 35.5841 196.54 29.8143 204.631 29.8143ZM260.005 34.6122C260.005 38.3257 258.48 41.3555 255.621 43.4835C258.48 45.6115 260.005 48.6413 260.004 52.3545C260.004 62.2781 250.687 65.7992 242.708 65.7992C236.321 65.7992 230.151 63.4199 227.354 59.8783L227.078 59.5277L233.081 53.8949L233.424 54.2537C234.959 55.8634 238.52 57.596 242.708 57.596C247.469 57.596 250.545 55.5386 250.545 52.3546C250.545 50.2835 249.086 48.4402 246.828 47.6585C246.412 47.5168 245.986 47.4056 245.553 47.3261H234.369V39.6405H245.553C245.986 39.561 246.412 39.4499 246.828 39.308C249.087 38.5265 250.545 36.6832 250.545 34.6122C250.545 31.4281 247.469 29.3708 242.708 29.3708C238.52 29.3708 234.959 31.1033 233.424 32.713L233.081 33.0719L227.078 27.439L227.354 27.0882C230.151 23.5468 236.321 21.1674 242.708 21.1674C250.687 21.1674 260.005 24.6885 260.005 34.6122ZM68.1877 56.8586C65.3282 55.9812 62.828 54.2244 61.0525 51.8448C59.277 49.4653 58.3192 46.5878 58.3192 43.6328C58.3192 40.6779 59.277 37.8004 61.0525 35.4209C62.828 33.0413 65.3282 31.2845 68.1877 30.407V21.3834C62.9183 22.3442 58.1563 25.0957 54.7286 29.1601C51.301 33.2246 49.4242 38.3452 49.4242 43.6329C49.4242 48.9205 51.301 54.0412 54.7286 58.1056C58.1563 62.1701 62.9183 64.9216 68.1877 65.8824V56.8586ZM76.5133 30.4071V21.3834C81.7828 22.3442 86.5448 25.0957 89.9724 29.1601C93.4 33.2246 95.2768 38.3452 95.2768 43.6329C95.2768 48.9205 93.4 54.0412 89.9724 58.1056C86.5448 62.1701 81.7828 64.9216 76.5133 65.8824V56.8587C79.3729 55.9813 81.8731 54.2244 83.6486 51.8449C85.4241 49.4653 86.3818 46.5878 86.3818 43.6329C86.3818 40.6779 85.4241 37.8004 83.6486 35.4209C81.8731 33.0414 79.3729 31.2845 76.5133 30.4071Z"
        fill={fill}
      />
    </svg>
  );
}

export default Sourc3Logo;
