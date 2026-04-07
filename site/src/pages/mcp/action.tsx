import type {ReactNode} from 'react';
import {Redirect} from '@docusaurus/router';

export default function McpAction(): ReactNode {
  return <Redirect to="/hardproof/ci" />;
}
